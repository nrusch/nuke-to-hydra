#ifndef HDNUKE_GEOADAPTER_H
#define HDNUKE_GEOADAPTER_H

#include <pxr/pxr.h>

#include <pxr/base/gf/vec2f.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <DDImage/GeoInfo.h>

#include "adapter.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HdNukeGeoAdapter : public HdNukeAdapter
{
public:
    HdNukeGeoAdapter(AdapterSharedState* statePtr);

    void Update(const GeoInfo& geo,
                HdDirtyBits dirtyBits = HdChangeTracker::AllDirty);

    inline GfMatrix4d GetTransform() const { return _transform; }

    inline GfRange3d GetExtent() const { return _extent; }

    inline HdMeshTopology GetMeshTopology() const { return _topology; }

    VtValue Get(const TfToken& key) const;

    SdfPath GetMaterialId(const SdfPath& rprimId) const;

    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(HdInterpolation interpolation) const;

private:
    void _RebuildPointList(const GeoInfo& geo);
    void _RebuildPrimvars(const GeoInfo& geo);
    void _RebuildMeshTopology(const GeoInfo& geo);

    template <typename T>
    inline void _StorePrimvarScalar(TfToken& key, const T& value) {
        _primvarData.emplace(key, VtValue(value));
    }

    inline void _StorePrimvarArray(TfToken& key, VtValue&& array) {
        _primvarData.emplace(key, std::move(array));
    }

    GfMatrix4d _transform;
    GfRange3d _extent;

    VtVec3fArray _points;
    VtVec2fArray _uvs;

    HdMeshTopology _topology;

    HdPrimvarDescriptorVector _constantPrimvarDescriptors;
    HdPrimvarDescriptorVector _uniformPrimvarDescriptors;
    HdPrimvarDescriptorVector _vertexPrimvarDescriptors;
    HdPrimvarDescriptorVector _faceVaryingPrimvarDescriptors;

    std::unordered_map<TfToken, VtValue, TfToken::HashFunctor> _primvarData;
};

using HdNukeGeoAdapterPtr = std::shared_ptr<HdNukeGeoAdapter>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_GEOADAPTER_H
