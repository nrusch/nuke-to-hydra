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
#ifndef HDNUKE_DELEGATECONFIG_H
#define HDNUKE_DELEGATECONFIG_H

#include <pxr/pxr.h>

#include <pxr/usd/sdf/path.h>


PXR_NAMESPACE_OPEN_SCOPE


class HdNukeDelegateConfig
{
public:
    HdNukeDelegateConfig();
    HdNukeDelegateConfig(const SdfPath& delegateId);

    inline const SdfPath& GeoRoot() const { return _geoRoot; }
    inline const SdfPath& LightRoot() const { return _lightRoot; }
    inline const SdfPath& MaterialRoot() const { return _materialRoot; }

    inline const SdfPath& NukeLightRoot() const { return _nukeLightRoot; }
    inline const SdfPath& HydraLightRoot() const { return _hydraLightRoot; }

    static const SdfPath DefaultDelegateID;

private:
    void _InitPaths();

    const SdfPath _delegateId;

    SdfPath _geoRoot;

    SdfPath _lightRoot;
    SdfPath _nukeLightRoot;
    SdfPath _hydraLightRoot;

    SdfPath _materialRoot;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_DELEGATECONFIG_H
