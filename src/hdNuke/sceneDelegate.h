#ifndef HDNUKE_SCENEDELEGATE_H
#define HDNUKE_SCENEDELEGATE_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <DDImage/GeoOp.h>
#include <DDImage/Scene.h>


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


static SdfPath DELEGATE_ID(TfToken("/Nuke_Scene"));
static SdfPath MESH_ROOT_ID = DELEGATE_ID.AppendChild(TfToken("Meshes"));


class HdNukeSceneDelegate : public HdSceneDelegate
{
public:
    HdNukeSceneDelegate(HdRenderIndex* renderIndex);

    ~HdNukeSceneDelegate() { }

    virtual bool IsEnabled(TfToken const& option) const;

    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);

    virtual GfRange3d GetExtent(SdfPath const & id);

    virtual GfMatrix4d GetTransform(SdfPath const & id);

    virtual VtValue Get(SdfPath const& id, TfToken const& key);

    void SyncFromGeoOp(GeoOp* op);
    void Clear();

private:
    Scene _scene;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_SCENEDELEGATE_H
