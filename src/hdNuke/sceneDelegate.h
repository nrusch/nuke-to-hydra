#ifndef HDNUKE_SCENEDELEGATE_H
#define HDNUKE_SCENEDELEGATE_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <DDImage/GeoOp.h>
#include <DDImage/Scene.h>

#include "geoAdapter.h"
#include "instancerAdapter.h"
#include "lightAdapter.h"
#include "sharedState.h"
#include "types.h"


using namespace DD::Image;


PXR_NAMESPACE_OPEN_SCOPE


class GfVec3f;


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

    VtIntArray GetInstanceIndices(const SdfPath& instancerId,
                                  const SdfPath& prototypeId) override;
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

    HdNukeGeoAdapterPtr GetGeoAdapter(const SdfPath& id) const;
    HdNukeInstancerAdapterPtr GetInstancerAdapter(const SdfPath& id) const;
    HdNukeLightAdapterPtr GetLightAdapter(const SdfPath& id) const;

    void SetDefaultDisplayColor(GfVec3f color);

    void SyncFromGeoOp(GeoOp* op);
    void SyncGeometry(GeometryList* geoList);
    void SyncLights(std::vector<LightContext*> lights);

    void ClearAll();
    void ClearGeo();
    void ClearLights();

    static uint32_t UpdateHashArray(const GeoOp* op, GeoOpHashArray& hashes);
    static HdDirtyBits DirtyBitsFromUpdateMask(uint32_t updateMask);

protected:
    void CreateOpGeo(GeoOp* geoOp, const SdfPath& subtree,
                     const GeoInfoVector& geoInfos);
    void UpdateOpGeo(GeoOp* geoOp, const SdfPath& subtree,
                     const GeoInfoVector& geoInfos);
    inline void _RemoveRprim(const SdfPath& primId);
    void _RemoveSubtree(const SdfPath& subtree);

private:
    Scene _scene;

    std::unordered_map<GeoOp*, SdfPath> _opSubtrees;
    std::unordered_map<GeoOp*, GeoOpHashArray> _opStateHashes;

    SdfPathMap<HdNukeGeoAdapterPtr> _geoAdapters;
    SdfPathMap<HdNukeInstancerAdapterPtr> _instancerAdapters;
    SdfPathMap<HdNukeLightAdapterPtr> _lightAdapters;

    AdapterSharedState sharedState;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_SCENEDELEGATE_H
