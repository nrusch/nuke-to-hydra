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
#ifndef HDNUKE_LIGHTADAPTER_H
#define HDNUKE_LIGHTADAPTER_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/types.h>

#include <DDImage/LightOp.h>

#include "adapter.h"


PXR_NAMESPACE_OPEN_SCOPE


class HdNukeLightAdapter : public HdNukeAdapter
{
public:
    HdNukeLightAdapter(AdapterSharedState* statePtr,
                       const DD::Image::LightOp* lightOp,
                       const TfToken& lightType);

    const DD::Image::LightOp* GetLightOp() const { return _light; }
    const TfToken& GetLightType() const { return _lightType; }

    const DD::Image::Hash& GetLastHash() const { return _lastHash; }
    inline void UpdateLastHash() { _lastHash = _light->hash(); }
    inline bool DirtyHash() const { return _lastHash != _light->hash(); }

    GfMatrix4d GetTransform() const;

    VtValue GetLightParamValue(const TfToken& paramName) const;

    static const HdDirtyBits DefaultDirtyBits;

private:
    const DD::Image::LightOp* _light;
    TfToken _lightType;
    DD::Image::Hash _lastHash;
};

using HdNukeLightAdapterPtr = std::shared_ptr<HdNukeLightAdapter>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_LIGHTADAPTER_H
