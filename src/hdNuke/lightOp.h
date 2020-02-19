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
#ifndef HDNUKE_HYDRALIGHTOP_H
#define HDNUKE_HYDRALIGHTOP_H

#include <pxr/pxr.h>

#include <DDImage/AxisOp.h>

#include "opBases.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HydraLightOp : public AxisOp, public HydraPrimOp
{
public:
    HydraLightOp(Node* node) : AxisOp(node), HydraPrimOp() { }

    void knobs(Knob_Callback f) override;
    int knob_changed(Knob* k) override;

    virtual void makeLightKnobs(Knob_Callback f);

    void Populate(HydraPrimOpManager* manager) override;

    GfMatrix4d GetTransform() const;

    virtual VtValue GetLightParamValue(const TfToken& paramName);

    static const HdDirtyBits DefaultDirtyBits;

private:
    // Knob storage
    float _intensity = 1.0f;
    float _exposure = 0.0f;
    float _color[3] = {1.0f, 1.0f, 1.0f};
    bool _normalize = false;
    float _diffuse = 1.0f;
    float _specular = 1.0f;
    bool _castShadows = true;
    float _shadowColor[3] = {0.0f, 0.0f, 0.0f};
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_HYDRALIGHTOP_H
