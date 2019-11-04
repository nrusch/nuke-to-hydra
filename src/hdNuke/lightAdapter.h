#ifndef HDNUKE_LIGHTADAPTER_H
#define HDNUKE_LIGHTADAPTER_H

#include <pxr/pxr.h>

#include <DDImage/LightOp.h>

#include "adapter.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HdNukeLightAdapter : public HdNukeAdapter
{
public:
    HdNukeLightAdapter(const SdfPath& id, AdapterSharedState* statePtr,
                       const LightOp* lightOp);

    const LightOp* GetLightOp() const { return _light; }

    GfMatrix4d GetTransform() const override;

    VtValue GetLightParamValue(const TfToken& paramName) const;

private:
    const LightOp* _light;
    Hash _lastHash;
};

using HdNukeLightAdapterPtr = std::shared_ptr<HdNukeLightAdapter>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_LIGHTADAPTER_H
