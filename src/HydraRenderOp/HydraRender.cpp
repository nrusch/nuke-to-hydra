// HydraRender
// By Nathan Rusch
//
// Renders a Nuke 3D scene using Hydra
//
#include <GL/glew.h>

#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/imaging/hdx/tokens.h>

#include <DDImage/CameraOp.h>
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/Iop.h>
#include <DDImage/Knob.h>
#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/Scene.h>
#include <DDImage/Thread.h>


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


static const char* const CLASS = "HydraRender";
static const char* const HELP = "Renders a Nuke 3D scene using Hydra.";


namespace {

bool g_pluginRegistryInitialized = false;
static TfTokenVector g_pluginIds;
static std::vector<std::string> g_pluginKnobStrings;


static void
scanRendererPlugins()
{
    if (!g_pluginRegistryInitialized) {
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
        TfDebug::Enable(HD_ENGINE_PHASE_INFO);

        g_pluginRegistryInitialized = true;
    }
}

}  // namespace


class HydraRender : public Iop {

public:
    HydraRender(Node* node)
        : Iop(node),
          _primCollection(TfToken("HydraNukeGeo"),
                          HdReprSelector(HdReprTokens->refined))
    {
        scanRendererPlugins();

        inputs(2);
    }

    ~HydraRender()
    {
        destroyRenderer();
    }

    const char* input_label(int index, char*) const
    {
        switch (index)
        {
            case 0:
                return "Scene/Geo";
            case 1:
                return "Camera";
        }
        return "XXX";
    }

    Op* default_input(int index) const override
    {
        if (index == 1) {
            return CameraOp::default_camera();
        }
        return nullptr;
    }

    bool test_input(int index, Op* op) const
    {
        switch (index)
        {
            case 0:
                return (dynamic_cast<Scene*>(op) != nullptr
                        or dynamic_cast<GeoOp*>(op) != nullptr);
            case 1:
                return dynamic_cast<CameraOp*>(op) != nullptr;
        }

        return false;
    }

    virtual void knobs(Knob_Callback f)
    {
        Format_knob(f, &_formats, "format");

        Knob* k = Enumeration_knob(f, &_selectedRenderer, 0, "renderer");
        SetFlags(f, Knob::ALWAYS_SAVE);
        if (f.makeKnobs()) {
            k->enumerationKnob()->menu(g_pluginKnobStrings);
        }
    }

    // TODO: Wrap this creation/deletion stuff in a struct
    void initRenderer()
    {
        _rendererPlugin = HdxRendererPluginRegistry::GetInstance().GetRendererPlugin(
            g_pluginIds[_selectedRenderer]);
        _renderDelegate = _rendererPlugin->CreateRenderDelegate();
        _renderIndex = HdRenderIndex::New(_renderDelegate);

        _taskController = new HdxTaskController(
            _renderIndex, SdfPath("/HydraNuke_TaskController"));
        _taskController->SetEnableSelection(false);
        _taskController->SetCollection(_primCollection);

        _rendererInitialized = true;
    }

    void destroyRenderer()
    {
        if (_taskController != nullptr) {
            delete _taskController;
            _taskController = nullptr;
        }

        if (_renderIndex != nullptr) {
            delete _renderIndex;
            _renderIndex = nullptr;
        }

        if (_rendererPlugin != nullptr) {
            if (_renderDelegate != nullptr) {
                _rendererPlugin->DeleteRenderDelegate(_renderDelegate);
                _renderDelegate = nullptr;
            }

            HdxRendererPluginRegistry::GetInstance().ReleasePlugin(_rendererPlugin);
            _rendererPlugin = nullptr;
        }

        _rendererInitialized = false;
    }

    void _validate(bool for_real)
    {
        info_.full_size_format(*_formats.fullSizeFormat());
        info_.format(*_formats.format());
        info_.channels(Mask_RGBA);
        info_.set(format());

        if (!_rendererInitialized) {
            initRenderer();
        }

        HdxRenderTaskParams renderTaskParams;
        renderTaskParams.enableLighting = true;
        renderTaskParams.enableSceneMaterials = true;
        _taskController->SetRenderParams(renderTaskParams);

        // To disable viewport rendering
        _taskController->SetViewportRenderOutput(TfToken());

        GfVec4d glviewport(0, 0, info_.w(), info_.h());
        _taskController->SetRenderViewport(glviewport);

        _needRender = true;
    }

    void _request(int x, int y, int r, int t, ChannelMask mask, int count)
    {
    }

    void engine(int y, int x, int r, ChannelMask channels, Row& out)
    {
        if (_needRender) {
            Guard g(_engineLock);
            if (_needRender) {
                auto tasks = _taskController->GetRenderingTasks();
                _engine.Execute(_renderIndex, &tasks);

                _needRender = false;

                while (!(aborted() or _taskController->IsConverged()))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
        }

        if (aborted()) {
            out.erase(channels);
            return;
        }

        HdRenderBuffer* colorBuffer = _taskController->GetRenderOutput(
            HdAovTokens->color);
        if (colorBuffer and colorBuffer->IsConverged()) {
            std::cerr << "Got a converged color buffer!" << std::endl;
            void* data = colorBuffer->Map();

            colorBuffer->Unmap();
        }
        else {
            out.erase(channels);
        }
    }

    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description desc;

private:
    HdRenderIndex* _renderIndex = nullptr;
    HdxRendererPlugin* _rendererPlugin = nullptr;  // Ref-counted by Hydra
    HdRenderDelegate* _renderDelegate = nullptr;

    HdEngine _engine;
    HdxTaskController* _taskController = nullptr;
    HdRprimCollection _primCollection;

    bool _rendererInitialized = false;
    bool _needRender = false;
    Lock _engineLock;

    // Knob storage
    int _selectedRenderer = 0;
    FormatPair _formats;
};

static Iop* build(Node* node) { return new HydraRender(node); }
const Iop::Description HydraRender::desc(CLASS, 0, build);


PXR_NAMESPACE_CLOSE_SCOPE
