#ifndef HDNUKE_ADAPTER_H
#define HDNUKE_ADAPTER_H

#include <pxr/pxr.h>

#include <pxr/usd/sdf/path.h>

#include "sharedState.h"


PXR_NAMESPACE_OPEN_SCOPE


class HdNukeAdapter
{
public:
    HdNukeAdapter(AdapterSharedState* statePtr)
        : _sharedState(statePtr) { }

    inline const AdapterSharedState* GetSharedState() const {
        return _sharedState;
    }

private:
    const AdapterSharedState* _sharedState;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_ADAPTER_H
