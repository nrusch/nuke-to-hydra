// Copyright 2019-present Nathan Rusch
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef HDNUKE_SCENEDELEGATE_H
#define HDNUKE_SCENEDELEGATE_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <pxr/usdImaging/usdImaging/delegate.h>

#include <DDImage/GeoOp.h>
#include <DDImage/Scene.h>

#include "delegateConfig.h"
#include "geoAdapter.h"
#include "instancerAdapter.h"
#include "lightAdapter.h"
#include "sharedState.h"
#include "types.h"


PXR_NAMESPACE_OPEN_SCOPE


class GfVec3f;

class HydraOp;
class HydraLightOp;
class HydraOpManager;

class HdNukeSceneDelegate : public HdSceneDelegate
{
public:
    HdNukeSceneDelegate(HdRenderIndex* renderIndex);
    HdNukeSceneDelegate(HdRenderIndex* renderIndex, const SdfPath& delegateId);

    ~HdNukeSceneDelegate() { }

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

    inline const HdNukeDelegateConfig& GetConfig() const { return _config; }

    TfToken GetRprimType(const DD::Image::GeoInfo& geoInfo) const;
    SdfPath GetRprimSubPath(const DD::Image::GeoInfo& geoInfo,
                            const TfToken& primType) const;

    inline const SdfPath& DefaultMaterialId() const { return _defaultMaterialId; }

    HdNukeGeoAdapterPtr GetGeoAdapter(const SdfPath& id) const;
    HdNukeInstancerAdapterPtr GetInstancerAdapter(const SdfPath& id) const;
    HdNukeLightAdapterPtr GetLightAdapter(const SdfPath& id) const;
    HydraLightOp* GetHydraLightOp(const SdfPath& id) const;

    void SetDefaultDisplayColor(GfVec3f color);

    void SyncFromGeoOp(DD::Image::GeoOp* geoOp);
    void SyncHydraOp(HydraOp* hydraOp);

    void ClearNukePrims();
    void ClearHydraPrims();
    void ClearAll();

    static uint32_t UpdateHashArray(const DD::Image::GeoOp* op,
                                    GeoOpHashArray& hashes);
    static HdDirtyBits DirtyBitsFromUpdateMask(uint32_t updateMask);

protected:
    void SyncNukeGeometry(DD::Image::GeometryList* geoList);
    void SyncNukeLights(std::vector<DD::Image::LightContext*> lights);

    void CreateOpGeo(DD::Image::GeoOp* geoOp, const SdfPath& subtree,
                     const GeoInfoVector& geoInfos);
    void UpdateOpGeo(DD::Image::GeoOp* geoOp, const SdfPath& subtree,
                     const GeoInfoVector& geoInfos);

    void ClearNukeGeo();
    void ClearNukeLights();


    inline void _RemoveRprim(const SdfPath& primId);
    void _RemoveSubtree(const SdfPath& subtree);

private:
    friend class HydraOpManager;

    HdNukeDelegateConfig _config;

    DD::Image::Scene _scene;

    std::unordered_map<DD::Image::GeoOp*, SdfPath> _opSubtrees;
    std::unordered_map<DD::Image::GeoOp*, GeoOpHashArray> _opStateHashes;

    SdfPathMap<HdNukeGeoAdapterPtr> _geoAdapters;
    SdfPathMap<HdNukeInstancerAdapterPtr> _instancerAdapters;
    SdfPathMap<HdNukeLightAdapterPtr> _lightAdapters;

    SdfPathMap<HydraLightOp*> _hydraLightOps;
    SdfPathMap<std::unique_ptr<UsdImagingDelegate>> _usdDelegates;

    AdapterSharedState sharedState;
    SdfPath _defaultMaterialId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_SCENEDELEGATE_H
