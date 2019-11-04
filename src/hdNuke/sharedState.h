#ifndef HDNUKE_SHAREDSTATE_H
#define HDNUKE_SHAREDSTATE_H

#include <pxr/pxr.h>

#include <pxr/base/gf/vec3f.h>


PXR_NAMESPACE_OPEN_SCOPE


// Container for common parameters that adapters may need access to.
struct AdapterSharedState
{
    GfVec3f defaultDisplayColor = {0.18, 0.18, 0.18};
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDNUKE_SHAREDSTATE_H
