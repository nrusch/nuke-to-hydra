// HydraRender
// By Nathan Rusch
//
// Renders a Nuke 3D scene using Hydra
//
#include <GL/glew.h>

#include <pxr/pxr.h>

#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/vec3f.h>

#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/imaging/hdx/tokens.h>

#include <pxr/usdImaging/usdImaging/delegate.h>

#include <DDImage/CameraOp.h>
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/PlanarIop.h>
#include <DDImage/Knob.h>
#include <DDImage/Knobs.h>
#include <DDImage/LUT.h>
#include <DDImage/Row.h>
#include <DDImage/Scene.h>
#include <DDImage/Thread.h>

#include <hdNuke/sceneDelegate.h>
#include <hdNuke/utils.h>


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


static const char* const CLASS = "HydraRender";
static const char* const HELP = "Renders a Nuke 3D scene using Hydra.";


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

        TfDebug::Enable(HD_BPRIM_ADDED);
        TfDebug::Enable(HD_TASK_ADDED);
        // TfDebug::Enable(HD_ENGINE_PHASE_INFO);

    });
}


struct HydraData
{
    HdRendererPlugin* rendererPlugin = nullptr;  // Ref-counted by Hydra
    HdRenderIndex* renderIndex = nullptr;
    // HdRenderDelegate* renderDelegate = nullptr;

    HdNukeSceneDelegate* nukeDelegate = nullptr;
    UsdImagingDelegate* usdDelegate = nullptr;

    HdxTaskController* taskController = nullptr;
    HdRprimCollection primCollection;

    HydraData(HdRendererPlugin* pluginPtr)
            : rendererPlugin(pluginPtr),
              primCollection(HdTokens->geometry,
                             HdReprSelector(HdReprTokens->refined))
    {
        HdRenderDelegate* renderDelegate = rendererPlugin->CreateRenderDelegate();
        renderIndex = HdRenderIndex::New(renderDelegate);

        nukeDelegate = new HdNukeSceneDelegate(renderIndex);

        static SdfPath taskControllerId("/HydraNuke_TaskController");
        taskController = new HdxTaskController(renderIndex, taskControllerId);

        taskController->SetCollection(primCollection);
    }

    ~HydraData()
    {
        std::cerr << "~HydraData" << std::endl;
        if (taskController != nullptr) {
            delete taskController;
            // taskController = nullptr;
        }

        if (usdDelegate != nullptr) {
            delete usdDelegate;
            // usdDelegate = nullptr;
        }

        if (nukeDelegate != nullptr) {
            delete nukeDelegate;
            // nukeDelegate = nullptr;
        }

        HdRenderDelegate* renderDelegate = nullptr;
        if (renderIndex != nullptr) {
            renderDelegate = renderIndex->GetRenderDelegate();
            delete renderIndex;
            // renderIndex = nullptr;
        }

        if (rendererPlugin != nullptr) {
            if (renderDelegate != nullptr) {
                rendererPlugin->DeleteRenderDelegate(renderDelegate);
                // renderDelegate = nullptr;
            }

            HdRendererPluginRegistry::GetInstance().ReleasePlugin(rendererPlugin);
            // rendererPlugin = nullptr;
        }
    }

    void loadUSDStage(const char* usdFilePath)
    {
        static SdfPath usdDelegateId("/USD_Scene");

        if (usdDelegate != nullptr) {
            renderIndex->RemoveSubtree(usdDelegateId, usdDelegate);
            // delete usdDelegate;
            // usdDelegate = nullptr;
        }

        if (strlen(usdFilePath) == 0) {
            return;
        }

        std::cerr << "HydraRender : loading stage " << usdFilePath << std::endl;
        if (auto stage = UsdStage::Open(usdFilePath)) {
            // XXX: Basic up-axis correction for sanity
            if (UsdGeomGetStageUpAxis(stage) == UsdGeomTokens->z) {
                if (auto defaultPrim = stage->GetDefaultPrim()) {
                    if (defaultPrim.IsA<UsdGeomXformable>()) {
                        stage->SetEditTarget(stage->GetSessionLayer());
                        auto xform = UsdGeomXformable(defaultPrim);
                        auto rotXop = xform.AddRotateXOp(
                            UsdGeomXformOp::PrecisionDouble,
                            TfToken("upAxisCorrection"));
                        rotXop.Set<double>(-90);
                    }
                }
            }

            TfTokenVector purposes;
            purposes.push_back(UsdGeomTokens->default_);
            purposes.push_back(UsdGeomTokens->render);

            UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), purposes, true);

            GfBBox3d bbox = bboxCache.ComputeWorldBound(stage->GetPseudoRoot());
            GfRange3d world = bbox.ComputeAlignedRange();

            GfVec3d worldCenter = (world.GetMin() + world.GetMax()) / 2.0;
            double worldSize = world.GetSize().GetLength();

            std::cerr << "worldCenter: " << worldCenter << "\n";
            std::cerr << "worldSize: " << worldSize << "\n";

            UsdPrim prim = stage->GetPseudoRoot();

            if (usdDelegate == nullptr) {
                usdDelegate = new UsdImagingDelegate(renderIndex, usdDelegateId);
            }
            usdDelegate->Populate(prim);
        }
    }

    static HydraData* Create(TfToken pluginId)
    {
        auto& pluginRegistry = HdRendererPluginRegistry::GetInstance();
        if (not pluginRegistry.IsRegisteredPlugin(pluginId)) {
            return nullptr;
        }

        HdRendererPlugin* plugin = pluginRegistry.GetRendererPlugin(pluginId);
        if (not plugin->IsSupported()) {
            pluginRegistry.ReleasePlugin(plugin);
            return nullptr;
        }

        return new HydraData(plugin);
    }
};


}  // namespace


class HydraRender : public PlanarIop
{
public:
    HydraRender(Node* node);
    ~HydraRender();

    const char* input_label(int index, char*) const override;
    Op* default_input(int index) const override;
    bool test_input(int index, Op* op) const override;

    void append(Hash& hash) override;

    void knobs(Knob_Callback f) override;
    int knob_changed(Knob* k) override;

    void _validate(bool for_real) override;
    bool renderFullPlanes() const override { return true; }
    void getRequests(const Box& box, const ChannelSet& channels, int count,
                     RequestOutput &reqData) const override { }
    void renderStripe(ImagePlane& plane) override;

    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description desc;

protected:
    inline HdxTaskController* taskController() const
    {
        return _hdata->taskController;
    }

    void initRenderer();
    std::vector<HdRenderBuffer*> getRenderBuffers() const;
    void copyBufferToImagePlane(HdRenderBuffer* buffer, ImagePlane& plane);

private:
    HdEngine _engine;
    std::unique_ptr<HydraData> _hdata;
    std::string _activeRenderer;
    bool _needRender = false;
    Lock _engineLock;

    // Knob storage
    FormatPair _formats;
    // We persist the selected renderer as a string rather than just an index,
    // since the available renderers may change between sessions.
    std::string _rendererId;
    int _rendererIndex = 0;
    float _displayColor[3] = {0.18, 0.18, 0.18};

    // Temp - for testing
    const char* _usdFilePath;
    bool _stagePathChanged = true;
};


static Iop* build(Node* node) { return new HydraRender(node); }
const Iop::Description HydraRender::desc(CLASS, 0, build);


// -----------------
// Op Implementation
// -----------------

HydraRender::HydraRender(Node* node)
        : PlanarIop(node),
          _usdFilePath("")
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
            return "Scene";
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
            return dynamic_cast<GeoOp*>(op) != nullptr;
        case 1:
            return dynamic_cast<CameraOp*>(op) != nullptr;
    }

    return false;
}

void
HydraRender::append(Hash& hash)
{
    // std::cerr << "HydraRender::append" << std::endl;
    Op::input(1)->append(hash);
    if (GeoOp* geoOp = dynamic_cast<GeoOp*>(Op::input(0))) {
        geoOp->append(hash);
    }
}

void
HydraRender::knobs(Knob_Callback f)
{
    Format_knob(f, &_formats, "format");

    String_knob(f, &_rendererId, "renderer_id");
    SetFlags(f, Knob::INVISIBLE | Knob::ALWAYS_SAVE | Knob::KNOB_CHANGED_ALWAYS);

    Knob* k = Enumeration_knob(f, &_rendererIndex, 0, "renderer");
    SetFlags(f, Knob::DO_NOT_WRITE | Knob::NO_RERENDER | Knob::EXPAND_TO_CONTENTS);
    if (f.makeKnobs()) {
        k->enumerationKnob()->menu(g_pluginKnobStrings);
    }

    Color_knob(f, _displayColor, "default_display_color", "default display color");

    File_knob(f, &_usdFilePath, "usd_file", "usd file");
}

int
HydraRender::knob_changed(Knob* k)
{
    // std::cerr << "HydraRender::knob_changed : " << k->name() << std::endl;
    if (k->is("usd_file")) {
        _stagePathChanged = true;
        return 1;
    }
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
        return 1;
    }
    if (k->is("default_display_color")) {
        if (_hdata->nukeDelegate) {
            _hdata->nukeDelegate->SetDefaultDisplayColor(GfVec3f(_displayColor));
        }
        return 1;
    }
    return Iop::knob_changed(k);
}

void
HydraRender::_validate(bool for_real)
{
    std::cerr << "HydraRender::_validate" << std::endl;

    initRenderer();

    if (not _hdata) {
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

    // const Matrix4& nukeMatrix = cam->matrix();
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

    if (GeoOp* geoOp = dynamic_cast<GeoOp*>(Op::input(0))) {
        geoOp->validate(for_real);
        _hdata->nukeDelegate->SyncFromGeoOp(geoOp);
    }
    else {
        _hdata->nukeDelegate->ClearAll();
    }

    _needRender = true;
}

void
HydraRender::renderStripe(ImagePlane& plane)
{
    Box box = plane.bounds();
    std::cerr << "HydraRender::renderStripe"  << std::endl;
    std::cerr << "  bounds: " << box.x() << ", " << box.y() << ", " << box.r() << ", " << box.t() << std::endl;
    std::cerr << "  channels: " << plane.channels() << " (packed: " << plane.packed() << ")" << std::endl;

    if (_needRender) {
        // XXX: For building/testing
        if (_stagePathChanged) {
            _hdata->loadUSDStage(_usdFilePath);
            _stagePathChanged = false;
        }


        auto tasks = taskController()->GetRenderingTasks();
        do {
            _engine.Execute(_hdata->renderIndex, &tasks);
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

std::vector<HdRenderBuffer*>
HydraRender::getRenderBuffers() const
{
    auto bprimIds = _hdata->renderIndex->GetBprimSubtree(
        HdPrimTypeTokens->renderBuffer, taskController()->GetControllerId());

    std::vector<HdRenderBuffer*> buffers;
    buffers.reserve(bprimIds.size());

    for (const auto& bprimId : bprimIds)
    {
        buffers.push_back(
            static_cast<HdRenderBuffer*>(_hdata->renderIndex->GetBprim(
                HdPrimTypeTokens->renderBuffer, bprimId)));
    }
    return buffers;
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
            convertRenderBufferData<int8_t>(data, dest, numPixels, numComponents,
                                            plane.packed());
            break;
        case HdFormatFloat16:
            convertRenderBufferData<GfHalf>(data, dest, numPixels, numComponents,
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
            convertRenderBufferData<int32_t>(data, dest, numPixels, numComponents,
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

void
HydraRender::initRenderer()
{
    if (_activeRenderer == _rendererId) {
        return;
    }
    auto* dataPtr = HydraData::Create(TfToken(_rendererId));
    _hdata.reset(dataPtr);
    _activeRenderer = _rendererId;
    if (dataPtr == nullptr) {
        return;
    }

    _hdata->nukeDelegate->SetDefaultDisplayColor(GfVec3f(_displayColor));

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


PXR_NAMESPACE_CLOSE_SCOPE
