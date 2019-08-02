// HydraRender
// By Nathan Rusch
//
// Renders a Nuke 3D scene using Hydra
//
#include <pxr/pxr.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>

#include "DDImage/CameraOp.h"
#include "DDImage/Iop.h"
#include "DDImage/Knobs.h"
#include "DDImage/Row.h"
#include "DDImage/Scene.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


static const char* const CLASS = "HydraRender";
static const char* const HELP = "Renders a Nuke 3D scene using Hydra.";


class HydraRender : public Iop {

public:
    HydraRender(Node* node) : Iop(node)
    {
        inputs(2);

        const auto& pluginRegistry = HdxRendererPluginRegistry::GetInstance();

        //.GetRendererPlugin(_rendererDesc.rendererName);
        //auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
        //_renderIndex = HdRenderIndex::New(renderDelegate);
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
    }

    void _validate(bool for_real)
    {
        // copy_info(0);
        set_out_channels(Mask_RGBA);
    }

    void _request(int x, int y, int r, int t, ChannelMask mask, int count)
    {
    }

    void engine(int y, int x, int r, ChannelMask channels, Row& out)
    {
        out.erase(channels);
    }

    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description desc;
};

static Iop* build(Node* node) { return new HydraRender(node); }
const Iop::Description HydraRender::desc(CLASS, 0, build);


PXR_NAMESPACE_CLOSE_SCOPE
