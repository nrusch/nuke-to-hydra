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

#endif  // HDNUKE_SHAREDSTATE_H
