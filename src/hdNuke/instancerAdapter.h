#ifndef HDNUKE_INSTANCERADAPTER_H
#define HDNUKE_INSTANCERADAPTER_H

#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/value.h>

#include "adapter.h"
#include "types.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HdNukeInstancerAdapter : public HdNukeAdapter
{
public:
    HdNukeInstancerAdapter(AdapterSharedState* statePtr)
        : HdNukeAdapter(statePtr) { }

    void Update(const GeoInfoVector& geoInfoPtrs);

    VtValue Get(const TfToken& key) const;

    inline size_t InstanceCount() const { return _instanceXforms.size(); }

private:
    VtMatrix4dArray _instanceXforms;
};

using HdNukeInstancerAdapterPtr = std::shared_ptr<HdNukeInstancerAdapter>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_INSTANCERADAPTER_H
