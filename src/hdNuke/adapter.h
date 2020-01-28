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
