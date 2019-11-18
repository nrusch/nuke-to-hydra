#ifndef HDNUKE_SCENEDELEGATE_H
#define HDNUKE_SCENEDELEGATE_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <DDImage/GeoOp.h>
#include <DDImage/Scene.h>

#include "geoAdapter.h"
#include "lightAdapter.h"
#include "sharedState.h"


namespace std
{
    template<>
    struct hash<Hash>
    {
        size_t operator()(const Hash& h) const
        {
            return h.value();
        }
    };
}  // namespace std


using namespace DD::Image;


PXR_NAMESPACE_OPEN_SCOPE


class GfVec3f;
class HdRenderIndex;


using GeoOpHashArray = std::array<Hash, Group_Last>;
using GeoInfoVector = std::vector<const GeoInfo*>;

template <typename T>
using SdfPathMap = std::unordered_map<SdfPath, T, SdfPath::Hash>;


class HdNukeSceneDelegate : public HdSceneDelegate
{
public:
    HdNukeSceneDelegate(HdRenderIndex* renderIndex);

    ~HdNukeSceneDelegate() { }



    }

    HdMeshTopology GetMeshTopology(const SdfPath& id) override;

    GfRange3d GetExtent(const SdfPath& id) override;

    GfMatrix4d GetTransform(const SdfPath& id) override;

    bool GetVisible(const SdfPath& id) override;

    VtValue Get(const SdfPath& id, const TfToken& key) override;

    SdfPath GetMaterialId(const SdfPath& rprimId) override;

    VtValue GetMaterialResource(const SdfPath& materialId) override;


    VtValue GetLightParamValue(const SdfPath &id,
                               const TfToken& paramName) override;

    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(const SdfPath& id,
                          HdInterpolation interpolation) override;

    TfToken GetRprimType(const GeoInfo& geoInfo) const;
    SdfPath GetRprimSubPath(const GeoInfo& geoInfo,
                            const TfToken& primType) const;

    uint32_t UpdateHashArray(const GeoOp* op, GeoOpHashArray& hashes) const;

    HdNukeGeoAdapterPtr GetGeoAdapter(const SdfPath& id) const;
    HdNukeLightAdapterPtr GetLightAdapter(const SdfPath& id) const;

    void SetDefaultDisplayColor(GfVec3f color);

    void SyncFromGeoOp(GeoOp* op);
    void SyncGeometry(GeoOp* op, GeometryList* geoList);
    void SyncLights(std::vector<LightContext*> lights);

    void ClearAll();
    void ClearGeo();
    void ClearLights();

protected:
    void CreateOpGeo(GeoOp* geoOp, const SdfPath& subtree,
                     const GeoInfoVector& geoInfos);
    void UpdateOpGeo(GeoOp* geoOp, const SdfPath& subtree,
                     const GeoInfoVector& geoInfos);

private:
    Scene _scene;

    // May not want to store op pointers at all...
    std::unordered_map<GeoOp*, SdfPath> _opSubtrees;
    std::unordered_map<GeoOp*, GeoOpHashArray> _opStateHashes;

    // Map GeoInfo.src_id() to prim ID
    std::unordered_map<Hash, SdfPath> _geoInfoPrimIds;

    SdfPathMap<HdNukeGeoAdapterPtr> _geoAdapters;
    SdfPathMap<HdNukeLightAdapterPtr> _lightAdapters;

    AdapterSharedState sharedState;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_SCENEDELEGATE_H
