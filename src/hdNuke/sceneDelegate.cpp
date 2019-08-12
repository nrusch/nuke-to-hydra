#include "sceneDelegate.h"


PXR_NAMESPACE_OPEN_SCOPE


HdNukeSceneDelegate::HdNukeSceneDelegate(HdRenderIndex* renderIndex)
    : HdSceneDelegate(renderIndex, DELEGATE_ID)
{

}

void
HdNukeSceneDelegate::SyncFromGeoOp(GeoOp* op)
{
    TF_VERIFY(op);

    if (not op->valid()) {
        TF_CODING_ERROR("SyncFromGeoOp called with invalid GeoOp");
        return;
    }

    op->build_scene(_scene);

    const GeometryList* geoList = _scene.object_list();

    std::cerr << "HdNukeSceneDelegate::SyncFromGeoOp : Scene built with " << geoList->size() << " GeoInfos" << std::endl;

    GetRenderIndex().InsertRprim(HdPrimTypeTokens->mesh, this,
                                 MESH_ROOT_ID.AppendChild(TfToken("someTestMesh")));
}

void
HdNukeSceneDelegate::Clear()
{
    std::cerr << "HdNukeSceneDelegate::Clear" << std::endl;
    GetRenderIndex().RemoveSubtree(MESH_ROOT_ID, this);
}

PXR_NAMESPACE_CLOSE_SCOPE
