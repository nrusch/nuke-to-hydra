#ifndef HDNUKE_TOKENS_H
#define HDNUKE_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/base/tf/staticTokens.h"


PXR_NAMESPACE_OPEN_SCOPE


#define HDNUKE_TOKENS                       \
    (Geo)                                   \
    (Lights)                                \
    (Materials)                             \
    ((defaultSurface, "__defaultSurface"))  \
    (st)                                    \
    /* Attribute name constants */          \
    /* in DDImage/Attribute.h */            \
    (Cf)                                    \
    (N)                                     \
    (name)                                  \
    (PW)                                    \
    (size)                                  \
    (transform)                             \
    (uv)                                    \
    (vel)


TF_DECLARE_PUBLIC_TOKENS(HdNukeTokens, HD_API, HDNUKE_TOKENS);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_TOKENS_H
