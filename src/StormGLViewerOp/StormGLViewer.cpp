#include <GL/glew.h>

#include <pxr/pxr.h>

#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/glContext.h"

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/renderIndex.h>
#include "pxr/imaging/hdSt/renderDelegate.h"
#include <pxr/imaging/hdx/taskController.h>

#include <DDImage/CameraOp.h>
#include <DDImage/NoIop.h>

// #include <hdNuke/sceneDelegate.h>
#include <hdNuke/utils.h>


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


static const char* const CLASS = "StormGLViewer";
static const char* const HELP = "Renders stuff to the 3D viewer using Storm.";


class StormGLViewer : public NoIop
{
public:
    StormGLViewer(Node* node);
    ~StormGLViewer();

    HandlesMode doAnyHandles(ViewerContext* ctx) override
    {
        if (ctx->transform_mode() != VIEWER_2D) {
            return eHandlesCooked;
        }

        return NoIop::doAnyHandles(ctx);
    }

    void setupRenderer();

    void build_handles(ViewerContext*) override;
    void draw_handle(ViewerContext*) override;

    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description desc;

private:
    bool _haveRunSetup = false;

    // The Render Delegate. Currently the constructor will always instantiate
    // a HdStRenderDelegate for it, but in order to support different renderers
    // this should be changed.
    HdRenderDelegate* m_renderDelegate;

    // The Render Index that will be populated in the ViewerDelegateComponent.
    HdRenderIndex* m_renderIndex;

    // The different objects used in the rendering of the scene.
    HdEngine                       m_engine;
    HdxTaskController*             m_taskController = nullptr;
    // HdxSelectionTrackerSharedPtr   m_selectionTracker;
    // GlfSimpleLightingContextRefPtr m_lightingContext;
    // HdxPickTaskContextParams       m_pickParams;
    HdRprimCollection              m_geoCollection;
};


static Iop* build(Node* node) { return new StormGLViewer(node); }
const Iop::Description StormGLViewer::desc(CLASS, 0, build);


// -----------------
// Op Implementation
// -----------------

StormGLViewer::StormGLViewer(Node* node) : NoIop(node)
{
    m_renderDelegate = new HdStRenderDelegate();
    m_renderIndex = HdRenderIndex::New(m_renderDelegate);
}

StormGLViewer::~StormGLViewer()
{
    if (m_renderIndex != nullptr) {
        delete m_renderIndex;
    }

    if (m_renderDelegate != nullptr) {
        delete m_renderDelegate;
    }
}

void
StormGLViewer::setupRenderer()
{
    _haveRunSetup = true;

    // Check the GL context and Hydra
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!context) {
        std::cerr << "OpenGL context required, cannot render" << std::endl;
        return;
    }

    GlfContextCaps::InitInstance();

    if (!HdStRenderDelegate::IsSupported())
    {
        std::cerr << "Current GL context doesn't support Hydra" << std::endl;
        return;
    }

    // if (TfGetenv("HD_ENABLED", "1") != "1")
    // {
    //     TF_DEBUG(KATANA_HYDRA).Msg("HD_ENABLED not enabled.");
    //     return;
    // }

    // Make the GL Context current
    GlfGLContext::MakeCurrent(context);

    // Init GLEW
    GlfGlewInit();

    // Create the Task controller
    m_taskController = new HdxTaskController(m_renderIndex,
        SdfPath("/HydraNuke_StormGLViewer_TaskController"));
    // m_taskController->SetEnableSelection(true);
    // m_taskController->SetSelectionColor(m_selectionColor);

    // Task Params
    HdxRenderTaskParams renderTaskParams;
    renderTaskParams.enableLighting = true;
    m_taskController->SetRenderParams(renderTaskParams);

    // Render Tags
    TfTokenVector renderTags;
    renderTags.push_back(HdRenderTagTokens->geometry);
    renderTags.push_back(HdRenderTagTokens->proxy);
    // NOTE: in order to render in full res use this instead of
    // HdRenderTagTokens->proxy:
    // renderTags.push_back(HdRenderTagTokens->render);
    m_taskController->SetRenderTags(renderTags);

    // Selection Tracker
    // m_selectionTracker.reset(new HdxSelectionTracker());

    // Lighting Context
    // m_lightingContext = initLighting();

    // Create the collection with all the geometry
    m_geoCollection = HdRprimCollection(
        TfToken("StormGLViewerGeo"),
        HdReprSelector(HdReprTokens->smoothHull));
    m_geoCollection.SetRootPath(SdfPath::AbsoluteRootPath());

    // Set the Collection in various entities
    m_taskController->SetCollection(m_geoCollection);
    m_renderIndex->GetChangeTracker().AddCollection(m_geoCollection.GetName());
}

// This method is called to build a list of things to call to actually draw the viewer
// overlay. The reason for the two passes is so that 3D movements in the viewer can be
// faster by not having to call every node, but only the ones that asked for draw_handle
// to be called:
void
StormGLViewer::build_handles(ViewerContext* vctx)
{
    // Cause any input iop's to draw (you may want to skip this depending on what you do)
    build_input_handles(vctx);

    // Cause any knobs to draw (we don't have any so this makes no difference):
    // build_knob_handles(ctx);

    // Don't draw anything unless viewer is in 3d mode:
    if (vctx->transform_mode() == VIEWER_2D) {
        return;
    }


    if (not _haveRunSetup) {
        setupRenderer();
    }

    // Something did not go well in setup
    if (m_taskController == nullptr) {
        return;
    }

    // make it call draw_handle():
    add_draw_handle(vctx);

    // Add our volume to the bounding box, so 'f' works to include this object:
    // float r = size * T;
    // ctx->expand_bbox(node_selected(), r, r, r);
    // ctx->expand_bbox(node_selected(), -r, -r, -r);
}

// And then it will call this when stuff needs to be drawn:
void
StormGLViewer::draw_handle(ViewerContext* vctx)
{
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    // Set the Lighting State
    // m_taskController->SetLightingState(m_lightingContext);

    GfMatrix4d camMatrix = DDToGfMatrix4d(vctx->cam_matrix());
    GfMatrix4d projMatrix = DDToGfMatrix4d(vctx->proj_matrix());

    // Camera
    // auto camera = viewport->getActiveCamera();

    // Camera Matrices
    // GfMatrix4d projMatrix = toGfMatrixd(camera->getProjectionMatrix());
    // GfMatrix4d viewMatrix = toGfMatrixd(camera->getViewMatrix());
    // m_taskController->SetFreeCameraMatrices(viewMatrix, projMatrix);
    m_taskController->SetFreeCameraMatrices(camMatrix, projMatrix);

    const Box& viewport = vctx->viewport();

    // Viewport size
    GfVec4d glviewport(0, 0, viewport.w(), viewport.h());
    m_taskController->SetRenderViewport(glviewport);

    // Sync collection with viewer display mode
    // syncDisplayMode(viewport);

    // Engine Selection State
    // VtValue selectionValue(m_selectionTracker);
    // m_engine.SetTaskContextData(HdxTokens->selectionState, selectionValue);

    // Render
    auto tasks = m_taskController->GetRenderingTasks();
    m_engine.Execute(m_renderIndex, &tasks);
}


PXR_NAMESPACE_CLOSE_SCOPE
