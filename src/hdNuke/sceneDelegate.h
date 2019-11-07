#ifndef HDNUKE_SCENEDELEGATE_H
#define HDNUKE_SCENEDELEGATE_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <DDImage/GeoOp.h>
#include <DDImage/Scene.h>

#include "geoAdapter.h"
#include "lightAdapter.h"
#include "sharedState.h"


using namespace DD::Image;





PXR_NAMESPACE_OPEN_SCOPE


class GfVec3f;
class HdRenderIndex;


class HdNukeSceneDelegate : public HdSceneDelegate
{
public:
    HdNukeSceneDelegate(HdRenderIndex* renderIndex);

    ~HdNukeSceneDelegate() { }




    // Currently this is only called with HdOptionTokens->parallelRprimSync.
    bool IsEnabled(const TfToken& option) const override {
        return false;
    }

    HdMeshTopology GetMeshTopology(const SdfPath& id) override;

    GfMatrix4d GetTransform(const SdfPath& id) override;

    VtValue Get(const SdfPath& id, const TfToken& key) override;

    SdfPath GetMaterialId(const SdfPath& rprimId) override;

    VtValue GetMaterialResource(const SdfPath& materialId) override;

    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(const SdfPath& id,
                          HdInterpolation interpolation) override;

    VtValue GetLightParamValue(const SdfPath &id,
                               const TfToken& paramName) override;


    SdfPath MakeRprimId(const GeoInfo& geoInfo) const;

    HdNukeGeoAdapterPtr GetGeoAdapter(const SdfPath& id) const;
    HdNukeLightAdapterPtr GetLightAdapter(const SdfPath& id) const;

    void SetDefaultDisplayColor(GfVec3f color);

    void SyncFromGeoOp(GeoOp* op);
    void SyncGeometry(GeoOp* op, GeometryList* geoList);
    void SyncLights(std::vector<LightContext*> lights);

    void ClearAll();
    void ClearGeo();
    void ClearLights();

private:
    Scene _scene;
    std::array<Hash, Group_Last> _geoHashes;

    template <typename T>
        using SdfPathMap = std::unordered_map<SdfPath, T, SdfPath::Hash>;
    SdfPathMap<HdNukeGeoAdapterPtr> _geoAdapters;
    SdfPathMap<HdNukeLightAdapterPtr> _lightAdapters;

    AdapterSharedState sharedState;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_SCENEDELEGATE_H
