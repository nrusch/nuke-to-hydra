// HydraRender
// By Nathan Rusch
//
// Renders a Nuke 3D scene using Hydra
//
#include <GL/glew.h>

#include <pxr/pxr.h>

#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/frustum.h>

#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>

#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/imaging/hdx/tokens.h>

#include <pxr/usdImaging/usdImaging/delegate.h>

#include <DDImage/CameraOp.h>
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/Iop.h>
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
        HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&plugins);

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
    HdRenderIndex* renderIndex = nullptr;
    HdxRendererPlugin* rendererPlugin = nullptr;  // Ref-counted by Hydra
    // HdRenderDelegate* renderDelegate = nullptr;

    HdNukeSceneDelegate* nukeDelegate = nullptr;
    UsdImagingDelegate* usdDelegate = nullptr;

    HdxTaskController* taskController = nullptr;
    HdRprimCollection primCollection;

    HydraData(TfToken pluginId)
            : primCollection(TfToken("HydraNukeGeo"),
                             HdReprSelector(HdReprTokens->refined))
    {
        std::cerr << "new HydraData" << std::endl;
        rendererPlugin = HdxRendererPluginRegistry::GetInstance()
            .GetRendererPlugin(pluginId);

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

            HdxRendererPluginRegistry::GetInstance().ReleasePlugin(rendererPlugin);
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
};


}  // namespace


class HydraRender : public Iop
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
    void _request(int x, int y, int r, int t, ChannelMask mask, int count) override { }
    void engine(int y, int x, int r, ChannelMask channels, Row& out) override;

    inline HdxTaskController* taskController() { return _hdata->taskController; }

    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description desc;

protected:
    void resetRenderer();

private:
    HdEngine _engine;
    std::unique_ptr<HydraData> _hdata;
    bool _needRender = false;
    Lock _engineLock;

    // Knob storage
    int _selectedRenderer = 0;
    FormatPair _formats;

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
        : Iop(node),
          _usdFilePath("")
{
    _scanRendererPlugins();
    resetRenderer();

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

    Knob* k = Enumeration_knob(f, &_selectedRenderer, 0, "renderer");
    SetFlags(f, Knob::ALWAYS_SAVE);
    if (f.makeKnobs()) {
        k->enumerationKnob()->menu(g_pluginKnobStrings);
    }

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
        resetRenderer();
        return 1;
    }
    return Iop::knob_changed(k);
}

void
HydraRender::_validate(bool for_real)
{
    std::cerr << "HydraRender::_validate" << std::endl;

    info_.full_size_format(*_formats.fullSizeFormat());
    info_.format(*_formats.format());
    info_.channels(Mask_RGBA);
    info_.set(format());

    GfVec4d viewport(0, 0, info_.w(), info_.h());
    taskController()->SetRenderViewport(viewport);

    // Set up Gf camera from camera input
    CameraOp* cam = dynamic_cast<CameraOp*>(Op::input(1));
    cam->validate(for_real);

    // const Matrix4& nukeMatrix = cam->matrix();
    GfMatrix4d camGfMatrix = DDToGfMatrix(cam->matrix());

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
        _hdata->nukeDelegate->Clear();
    }

    _needRender = true;
}

void
HydraRender::engine(int y, int x, int r, ChannelMask channels, Row& out)
{
    if (_needRender) {
        Guard g(_engineLock);
        if (_needRender) {
            std::cerr << "HydraRender::engine"  << std::endl;
            std::cerr << "  channels: " << channels << std::endl;

            // XXX: For building/testing
            if (_stagePathChanged) {
                _hdata->loadUSDStage(_usdFilePath);
                _stagePathChanged = false;
            }

            auto tasks = taskController()->GetRenderingTasks();
            _engine.Execute(_hdata->renderIndex, &tasks);

            while (!taskController()->IsConverged())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(75));
            }

            if (!aborted()) {

                HdRenderBuffer* colorBuffer = taskController()->GetRenderOutput(
                    HdAovTokens->color);
                if (colorBuffer != nullptr) {
                    colorBuffer->Resolve();
                    std::cerr << "Buffer: "
                        << colorBuffer->GetWidth() << 'x' << colorBuffer->GetHeight()
                        << ", format: " << colorBuffer->GetFormat() << std::endl;
                }
                else {
                    error("Null color buffer after render!");
                }

                _needRender = false;
            }
        }
    }

    if (aborted()) {
        out.erase(channels);
        return;
    }

    HdRenderBuffer* colorBuffer = taskController()->GetRenderOutput(
        HdAovTokens->color);
    if (colorBuffer) {
        out.erase(channels);  // XXX: Temp

        if (colorBuffer->GetFormat() == HdFormatUNorm8Vec4) {
            void* data = colorBuffer->Map();
            foreach(z, channels) {
                float* dest = out.writable(z) + x;
                Linear::from_byte(
                    dest,
                    static_cast<uint8_t*>(data) + (y * 4 * colorBuffer->GetWidth()) + (x * 4) + colourIndex(z),
                    r - x,
                    4);
            }
            colorBuffer->Unmap();
        }
    }
    else {
        out.erase(channels);
    }
}

void
HydraRender::resetRenderer()
{
    // TODO: Check for same renderer plugin
    _hdata.reset(new HydraData(g_pluginIds[_selectedRenderer]));

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
