#ifndef HDNUKE_SCENEDELEGATE_H
#define HDNUKE_SCENEDELEGATE_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <DDImage/GeoOp.h>
#include <DDImage/Scene.h>

#include "utils.h"


using namespace DD::Image;



PXR_NAMESPACE_OPEN_SCOPE


typedef std::unordered_map<SdfPath, const GeoInfo*, SdfPath::Hash> RprimGeoInfoPtrMap;
typedef std::unordered_map<SdfPath, const GeoInfo&, SdfPath::Hash> RprimGeoInfoRefMap;
typedef std::unordered_map<SdfPath, const LightOp*, SdfPath::Hash> LightOpPtrMap;


class GfVec3f;


class HdNukeSceneDelegate : public HdSceneDelegate
{
public:
    HdNukeSceneDelegate(HdRenderIndex* renderIndex);

    ~HdNukeSceneDelegate() { }




    bool IsEnabled(const TfToken& option) const override
    {
        // Currently this is only called with HdOptionTokens->parallelRprimSync.
        return false;
    }

    HdMeshTopology GetMeshTopology(const SdfPath& id) override;

    GfMatrix4d GetTransform(const SdfPath& id) override;

    VtValue Get(const SdfPath& id, const TfToken& key) override;

    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation) override;

    SdfPath MakeRprimId(const GeoInfo& geoInfo) const;

    void SetDefaultDisplayColor(GfVec3f color);
    void SyncFromGeoOp(GeoOp* op);
    void SyncGeometry(GeoOp* op, GeometryList* geoList);
    void SyncLights(std::vector<LightContext*> lights);
    void ClearAll();
    void ClearGeo();

private:
    GfVec3f _defaultDisplayColor = {0.18, 0.18, 0.18};

    Scene _scene;
    std::array<Hash, Group_Last> _geoHashes;

    RprimGeoInfoPtrMap _rprimGeoInfos;
    LightOpPtrMap _lightOps;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_SCENEDELEGATE_H
