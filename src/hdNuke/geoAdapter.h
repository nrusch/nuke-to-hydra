#ifndef HDNUKE_GEOADAPTER_H
#define HDNUKE_GEOADAPTER_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <DDImage/GeoInfo.h>

#include "adapter.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HdNukeGeoAdapter : public HdNukeAdapter
{
public:
    HdNukeGeoAdapter(const SdfPath& id, AdapterSharedState* statePtr,
                     GeoInfo& geoInfo);

    GfMatrix4d GetTransform() const override;

    const GeoInfo* GetGeoInfo() const { return _geo; }

    GfRange3d GetExtent() const;

    HdMeshTopology GetMeshTopology() const;

    VtValue Get(const TfToken& key) const;

    SdfPath GetMaterialId(const SdfPath& rprimId) const;

    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(HdInterpolation interpolation) const;

private:
    GeoInfo* _geo;
};

using HdNukeGeoAdapterPtr = std::shared_ptr<HdNukeGeoAdapter>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_GEOADAPTER_H
