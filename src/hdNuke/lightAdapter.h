#ifndef HDNUKE_LIGHTADAPTER_H
#define HDNUKE_LIGHTADAPTER_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/types.h>

#include <DDImage/LightOp.h>

#include "adapter.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HdNukeLightAdapter : public HdNukeAdapter
{
public:
    HdNukeLightAdapter(const SdfPath& id, AdapterSharedState* statePtr,
                       const LightOp* lightOp, const TfToken& lightType);

    const LightOp* GetLightOp() const { return _light; }
    const TfToken& GetLightType() const { return _lightType; }

    const Hash& GetLastHash() const { return _lastHash; }
    inline void UpdateLastHash() { _lastHash = _light->hash(); }
    inline bool DirtyHash() const { return _lastHash != _light->hash(); }

    GfMatrix4d GetTransform() const override;

    VtValue GetLightParamValue(const TfToken& paramName) const;

    static HdDirtyBits DefaultDirtyBits;

private:
    const LightOp* _light;
    TfToken _lightType;
    Hash _lastHash;
};

using HdNukeLightAdapterPtr = std::shared_ptr<HdNukeLightAdapter>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_LIGHTADAPTER_H
