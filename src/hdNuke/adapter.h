#ifndef HDNUKE_ADAPTER_H
#define HDNUKE_ADAPTER_H

#include <pxr/pxr.h>

#include <pxr/usd/sdf/path.h>

#include "sharedState.h"


PXR_NAMESPACE_OPEN_SCOPE


class GfMatrix4d;


class HdNukeAdapter
{
public:
    HdNukeAdapter(const SdfPath& id, AdapterSharedState* statePtr)
        : _id(id), _sharedState(statePtr) { }
    virtual ~HdNukeAdapter() { }

    const SdfPath& GetId() const { return _id; }
    inline const AdapterSharedState* GetSharedState() const {
        return _sharedState;
    }

    virtual GfMatrix4d GetTransform() const = 0;

private:
    SdfPath _id;
    const AdapterSharedState* _sharedState;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_ADAPTER_H
