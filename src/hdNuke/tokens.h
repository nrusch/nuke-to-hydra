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
#ifndef HDNUKE_TOKENS_H
#define HDNUKE_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/base/tf/staticTokens.h"


PXR_NAMESPACE_OPEN_SCOPE


#define HDNUKE_TOKENS                       \
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

#define HDNUKE_PATH_TOKENS                  \
    (Geo)                                   \
    (Lights)                                \
    (Materials)                             \
    (Nuke)                                  \
    (Hydra)                                 \
    ((defaultSurface, "__defaultSurface"))


TF_DECLARE_PUBLIC_TOKENS(HdNukeTokens, HD_API, HDNUKE_TOKENS);

TF_DECLARE_PUBLIC_TOKENS(HdNukePathTokens, HD_API, HDNUKE_PATH_TOKENS);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_TOKENS_H
