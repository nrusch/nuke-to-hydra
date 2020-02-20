// Copyright 2019-present Nathan Rusch
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <GL/glew.h>

#include <pxr/pxr.h>

#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/vec3f.h>

#include <pxr/imaging/hd/engine.h>

#include <DDImage/CameraOp.h>
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/PlanarIop.h>
#include <DDImage/Knob.h>
#include <DDImage/Knobs.h>
#include <DDImage/LUT.h>
#include <DDImage/Row.h>
#include <DDImage/Scene.h>

#include <hdNuke/knobFactory.h>
#include <hdNuke/renderStack.h>
#include <hdNuke/utils.h>


using namespace DD::Image;
PXR_NAMESPACE_USING_DIRECTIVE


static const char* const CLASS = "HydraRender";
static const char* const HELP =
    "Renders a Nuke 3D scene using a Hydra render delegate.";


class HydraRender : public PlanarIop, public HdNukeKnobFactory
{
public:
    HydraRender(Node* node);
    ~HydraRender();

    const char* input_label(int index, char*) const override;
    Op* default_input(int index) const override;
    bool test_input(int index, Op* op) const override;

    void knobs(Knob_Callback f) override;
    int knob_changed(Knob* k) override;

    void append(Hash& hash) override;

    void _validate(bool for_real) override;

    bool renderFullPlanes() const override { return true; }

    void getRequests(const Box& box, const ChannelSet& channels, int count,
                     RequestOutput &reqData) const override { }

    void renderStripe(ImagePlane& plane) override;

    const char* Class() const override { return CLASS; }
    const char* node_help() const override { return HELP; }

    static const Iop::Description desc;

    void renderDelegateKnobCallback(Knob_Callback f);
    inline void syncRenderDelegateSettingKnob(Knob* k);

    static const std::string RENDERER_KNOB_PREFIX;
    static void dynamicKnobCallback(void* ptr, Knob_Callback f);

protected:
    inline HdNukeSceneDelegate* sceneDelegate() const {
        return _hydra->nukeDelegate;
    }

    inline HdRenderDelegate* renderDelegate() const {
        return _hydra->GetRenderDelegate();
    }

    inline HdxTaskController* taskController() const {
        return _hydra->taskController;
    }

    void initRenderer() { initRenderer(_rendererId); }
    void initRenderer(const std::string& delegateId);

    void copyBufferToImagePlane(HdRenderBuffer* buffer, ImagePlane& plane);

private:
    std::unique_ptr<HydraRenderStack> _hydra;
    HdEngine _engine;
    std::string _activeRenderer;
    bool _needRender = false;

    std::vector<std::string> _delegateKnobNames;
    std::unordered_map<std::string, HdRenderSettingDescriptor> _delegateSettings;

    // Knob storage
    FormatPair _formats;
    // We persist the selected renderer as a string rather than just an index,
    // since the available renderers may change between sessions.
    std::string _rendererId;
    int _rendererIndex = 0;
    float _displayColor[3] = {0.18, 0.18, 0.18};

    // The index of the first dynamic render delegate knob.
    int _renderDelegateKnobStartIndex = -1;
    // The number of dynamic render delegate knobs, to pass to `replace_knobs`.
    int _renderDelegateKnobCount = 0;
    bool _needDelegateKnobSync = true;
};


static Iop* build(Node* node) { return new HydraRender(node); }
const Iop::Description HydraRender::desc(CLASS, 0, build);

const std::string HydraRender::RENDERER_KNOB_PREFIX("rd_");


namespace {

static TfTokenVector g_pluginIds;
static std::vector<std::string> g_pluginKnobStrings;


static void
_scanRendererPlugins()
{
    static std::once_flag pluginInitFlag;

    std::call_once(pluginInitFlag, [] {
        HfPluginDescVector plugins;
        HdRendererPluginRegistry::GetInstance().GetPluginDescs(&plugins);

        g_pluginIds.reserve(plugins.size());
        g_pluginKnobStrings.reserve(plugins.size());
        std::ostringstream buf;

        for (const auto& pluginDesc : plugins)
        {
            std::cerr << "  " << pluginDesc.id << std::endl;

            // FIXME: Skipping Storm for now (GL-only)
            if (pluginDesc.id == "HdStormRendererPlugin") {
                continue;
            }

            g_pluginIds.push_back(pluginDesc.id);
            buf << pluginDesc.id.GetString() << '\t' << pluginDesc.displayName;
            g_pluginKnobStrings.push_back(buf.str());
            buf.str(std::string());
            buf.clear();
        }
    });
}

}  // namespace



// -----------------
// Op Implementation
// -----------------

HydraRender::HydraRender(Node* node)
        : PlanarIop(node)
{
    _scanRendererPlugins();

    if (not g_pluginIds.empty()) {
        _rendererId = g_pluginIds[0];
    }

    inputs(2);
}

HydraRender::~HydraRender()
{
}

const char*
HydraRender::input_label(int index, char*) const
{
    switch (index)
    {
        case 0:
            return "Nuke Scene";
        case 1:
            return "Camera";
    }
    return "XXX";
}

Op*
HydraRender::default_input(int index) const
{
    if (index == 1) {
        return CameraOp::default_camera();
    }
    return nullptr;
}

bool
HydraRender::test_input(int index, Op* op) const
{
    switch (index)
    {
        case 0:
            return op_cast<GeoOp*>(op) != nullptr;
        case 1:
            return dynamic_cast<CameraOp*>(op) != nullptr;
    }

    return false;
}

void
HydraRender::append(Hash& hash)
{
    hash.append(outputContext().frame());
    Op::input(1)->append(hash);
    if (GeoOp* geoOp = op_cast<GeoOp*>(Op::input(0))) {
        geoOp->append(hash);
    }
}

void
HydraRender::knobs(Knob_Callback f)
{
    Format_knob(f, &_formats, "format");

    String_knob(f, &_rendererId, "renderer_id");
    SetFlags(f, Knob::INVISIBLE | Knob::ALWAYS_SAVE | Knob::KNOB_CHANGED_ALWAYS
                | Knob::KNOB_CHANGED_RECURSIVE);

    Knob* k = Enumeration_knob(f, &_rendererIndex, 0, "renderer");
    SetFlags(f, Knob::DO_NOT_WRITE | Knob::NO_RERENDER | Knob::EXPAND_TO_CONTENTS);
    if (f.makeKnobs()) {
        k->enumerationKnob()->menu(g_pluginKnobStrings);
    }

    Color_knob(f, _displayColor, "default_display_color", "default display color");

    Button(f, "force_update", "force update");
    SetFlags(f, Knob::STARTLINE);

    BeginClosedGroup(f, "renderer_knob_group", "render delegate settings");
    if (f.makeKnobs()) {
        _renderDelegateKnobStartIndex = f.getKnobCount();
    }
    int delegateKnobCount = add_knobs(dynamicKnobCallback, firstOp(), f);
    if (f.makeKnobs()) {
        _renderDelegateKnobCount = delegateKnobCount;
    }
    EndGroup(f);
}

int
HydraRender::knob_changed(Knob* k)
{
    if (k->is("renderer")) {
        std::string newId = k->enumerationKnob()->getItemValueString(_rendererIndex);
        knob("renderer_id")->set_text(newId.c_str());
        return 1;
    }
    if (k->is("renderer_id")) {
        const char* newId = k->get_text();
        Knob* menu = knob("renderer");
        if (g_pluginIds[static_cast<size_t>(menu->get_value())] != newId) {
            size_t i = 0;
            for (const auto& pluginId : g_pluginIds) {
                if (pluginId == newId) {
                    menu->set_flag(Knob::NO_KNOB_CHANGED);
                    menu->set_value(i);
                    menu->clear_flag(Knob::NO_KNOB_CHANGED);
                    break;
                }
                i++;
            }
        }
        initRenderer(newId);
        FreeDynamicKnobStorage();
        _renderDelegateKnobCount = replace_knobs(knob("renderer_knob_group"),
                                                 _renderDelegateKnobCount,
                                                 dynamicKnobCallback, firstOp());
        _needDelegateKnobSync = true;
        return 1;
    }
    if (k->is("default_display_color")) {
        sceneDelegate()->SetDefaultDisplayColor(GfVec3f(_displayColor));
        return 1;
    }
    if (k->is("force_update")) {
        sceneDelegate()->ClearAll();
        invalidate();
        return 1;
    }
    if (k->startsWith(RENDERER_KNOB_PREFIX.c_str())) {
        syncRenderDelegateSettingKnob(k);
        return 1;
    }
    return Iop::knob_changed(k);
}

void
HydraRender::_validate(bool for_real)
{
    initRenderer();

    if (not _hydra) {
        if (_rendererId.empty()) {
            error("Empty renderer_id");
        }
        else {
            error("Renderer plugin %s is unavailable or unsupported in the "
                  "current environment", _rendererId.c_str());
        }
        return;
    }

    info_.full_size_format(*_formats.fullSizeFormat());
    info_.format(*_formats.format());
    info_.channels(Mask_RGBA | Mask_Z);
    info_.set(format());

    GfVec4d viewport(0, 0, info_.w(), info_.h());
    taskController()->SetRenderViewport(viewport);

    // Set up Gf camera from camera input
    CameraOp* cam = dynamic_cast<CameraOp*>(Op::input(1));
    cam->validate(for_real);

    if (GeoOp* geoOp = op_cast<GeoOp*>(Op::input(0))) {
        geoOp->validate(for_real);
    }

    GfMatrix4d camGfMatrix = DDToGfMatrix4d(cam->matrix());

    // TODO:
    // - Support horiz/vertical aperture offset (need to convert Nuke
    // normalized to USD physical units?)
    // - Support focus distance, fstop
    GfCamera gfCamera(
        camGfMatrix,
        cam->projection_mode() == CameraOp::LENS_ORTHOGRAPHIC ?
            GfCamera::Orthographic : GfCamera::Perspective,
        cam->film_width(),  // horizontalAperture
        cam->film_height(),  // verticalAperture
        0.0, 0.0,  // horizontal/vertical aperture offsets
        cam->focal_length(),  // focalLength
        GfRange1f(cam->Near(), cam->Far())  // clippingRange
    );

    GfFrustum frustum = gfCamera.GetFrustum();
    taskController()->SetFreeCameraMatrices(frustum.ComputeViewMatrix(),
                                            frustum.ComputeProjectionMatrix());

    if (_needDelegateKnobSync and _renderDelegateKnobCount > 0
            and _renderDelegateKnobStartIndex > 0)
    {
        const int lastIndex = _renderDelegateKnobStartIndex + _renderDelegateKnobCount;
        for (int ki = _renderDelegateKnobStartIndex; ki <= lastIndex; ki++)
        {
            Knob* delegateKnob = knob(ki);
            if (delegateKnob->not_default()) {
                syncRenderDelegateSettingKnob(delegateKnob);
            }
        }
        _needDelegateKnobSync = false;
    }

    if (for_real) {
        _needRender = true;
    }
}

void
HydraRender::renderStripe(ImagePlane& plane)
{
    if (_needRender) {
        if (GeoOp* geoOp = op_cast<GeoOp*>(Op::input(0))) {
            sceneDelegate()->SyncFromGeoOp(geoOp);
        }
        else {
            sceneDelegate()->ClearAll();
        }

        auto tasks = taskController()->GetRenderingTasks();
        do {
            _engine.Execute(_hydra->renderIndex, &tasks);
        }
        while (!taskController()->IsConverged());

        _needRender = false;

        if (!taskController()->GetRenderOutput(HdAovTokens->color)) {
            error("Null color buffer after render!");
            return;
        }
    }

    if (aborted()) {
        return;
    }

    plane.makeWritable();
    const ChannelSet channels = plane.channels();

    TfToken outputName;
    if (channels & Mask_RGBA) {
        outputName = HdAovTokens->color;
    }
    else if (channels & Mask_Z) {
        outputName = HdAovTokens->depth;
    }
    else {
        error("Unknown ChanneSet requested in renderStripe");
        return;
    }

    HdRenderBuffer* sourceBuffer = taskController()->GetRenderOutput(outputName);
    if (!sourceBuffer) {
        error("Could not find render buffer for output %s", outputName.GetText());
        return;
    }

    sourceBuffer->Resolve();
    copyBufferToImagePlane(sourceBuffer, plane);
}

void
HydraRender::initRenderer(const std::string& delegateId)
{
    if (delegateId == _activeRenderer) {
        return;
    }

    auto* dataPtr = HydraRenderStack::Create(TfToken(delegateId));
    _hydra.reset(dataPtr);
    _activeRenderer = delegateId;
    if (dataPtr == nullptr) {
        return;
    }

    sceneDelegate()->SetDefaultDisplayColor(GfVec3f(_displayColor));

    taskController()->SetEnableSelection(false);

    HdxRenderTaskParams renderTaskParams;
    renderTaskParams.enableLighting = true;
    renderTaskParams.enableSceneMaterials = true;
    taskController()->SetRenderParams(renderTaskParams);

    // To disable viewport rendering
    taskController()->SetViewportRenderOutput(TfToken());

    TfTokenVector renderTags;
    renderTags.push_back(HdRenderTagTokens->geometry);
    renderTags.push_back(HdRenderTagTokens->render);
    taskController()->SetRenderTags(renderTags);
}

void
HydraRender::renderDelegateKnobCallback(Knob_Callback f)
{
    if (not _hydra) {
        initRenderer();
        if (not _hydra) {
            return;
        }
    }

    if (f.makeKnobs()) {
        auto settingsDescriptors = renderDelegate()->GetRenderSettingDescriptors();
        _delegateKnobNames.clear();
        _delegateKnobNames.reserve(settingsDescriptors.size());
        _delegateSettings.clear();
        _delegateSettings.reserve(settingsDescriptors.size());

        for (const auto& setting : settingsDescriptors)
        {
            std::string knobName = setting.key.GetString();
            std::replace(knobName.begin(), knobName.end(), ' ', '_');
            knobName = RENDERER_KNOB_PREFIX + knobName;
            _delegateKnobNames.push_back(knobName);
            _delegateSettings.emplace(knobName, setting);
        }
    }

    for (const auto& knobName : _delegateKnobNames)
    {
        HdRenderSettingDescriptor setting = _delegateSettings[knobName];
        VtValueKnob(f, knobName, setting.name, setting.defaultValue);
        SetFlags(f, Knob::STARTLINE);
    }
}

inline void
HydraRender::syncRenderDelegateSettingKnob(Knob* k)
{
    VtValue newValue = KnobToVtValue(k);
    if (not newValue.IsEmpty()) {
        const auto& setting = _delegateSettings[k->name()];
        renderDelegate()->SetRenderSetting(setting.key, newValue);
    }
}

void
HydraRender::copyBufferToImagePlane(HdRenderBuffer* buffer, ImagePlane& plane)
{
    const HdFormat bufferFormat = buffer->GetFormat();
    const size_t numComponents = HdGetComponentCount(bufferFormat);

    const ChannelSet channels = plane.channels();
    if (channels.size() != numComponents) {
        error("Buffer component count (%zu) does not match output plane "
              "channel count (%d)", numComponents, channels.size());
        return;
    }

    const size_t numPixels = plane.bounds().area();
    void* data = buffer->Map();
    float* dest = plane.writable();

    switch (HdGetComponentFormat(bufferFormat)) {
        case HdFormatUNorm8:
            if (plane.packed() or numComponents == 1) {
                Linear::from_byte(dest, static_cast<uint8_t*>(data),
                                  numPixels * numComponents);
            }
            else {
                foreach(z, channels) {
                    const uint8_t chanOffset = colourIndex(z);
                    Linear::from_byte(
                        dest + numPixels * chanOffset,
                        static_cast<uint8_t*>(data) + chanOffset,
                        numPixels,
                        numComponents);
                }
            }
            break;
        case HdFormatSNorm8:
            ConvertHdBufferData<int8_t>(data, dest, numPixels, numComponents,
                                        plane.packed());
            break;
        case HdFormatFloat16:
            ConvertHdBufferData<GfHalf>(data, dest, numPixels, numComponents,
                                        plane.packed());
            break;
        case HdFormatFloat32:
            if (plane.packed() or numComponents == 1) {
                Linear::from_float(dest, static_cast<float*>(data),
                                   numPixels * numComponents);
            }
            else {
                foreach(z, channels) {
                    const uint8_t chanOffset = colourIndex(z);
                    Linear::from_float(
                        dest + numPixels * chanOffset,
                        static_cast<float*>(data) + chanOffset,
                        numPixels,
                        numComponents);
                }
            }
            break;
        case HdFormatInt32:
            ConvertHdBufferData<int32_t>(data, dest, numPixels, numComponents,
                                         plane.packed());
            break;
        default:
            TF_WARN("[HydraRender] Unhandled render buffer format: %d",
                    static_cast<std::underlying_type<HdFormat>::type>(bufferFormat));
            foreach(z, channels) {
                plane.fillChannel(z, 0.0f);
            }
    }

    buffer->Unmap();
}

/* static */
void
HydraRender::dynamicKnobCallback(void* ptr, Knob_Callback f)
{
    HydraRender* op = static_cast<HydraRender*>(ptr);
    op->renderDelegateKnobCallback(f);
}
