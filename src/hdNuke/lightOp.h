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
#include "vtValueKnobCache.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HydraLightOp : public AxisOp, public HydraPrimOp
{
public:
    HydraLightOp(Node* node,  const TfToken& primType)
        : AxisOp(node), HydraPrimOp(), _primType(primType) { }

    void knobs(Knob_Callback f) override;
    int knob_changed(Knob* k) override;

    const TfToken& GetPrimTypeName() const override { return _primType; }

    void Populate(HydraOpManager* manager) override;

    GfMatrix4d GetTransform() const;

    virtual VtValue GetLightParamValue(const TfToken& paramName);

    static const HdDirtyBits DefaultDirtyBits;

protected:
    virtual void MakeLightKnobs(Knob_Callback f);

    inline bool RegisterLightParamKnob(Knob_Callback f, const TfToken& paramName);

private:
    const TfToken& _primType;

    VtValueKnobCache _paramKnobCache;
    // XXX: Is there a better way of doing something with knob values one time
    // after the first knob store (in order to pick up non-default values),
    // without needing to explicitly reference their storage fields?
    bool _knobCachePopulated = false;

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


bool
HydraLightOp::RegisterLightParamKnob(Knob_Callback f, const TfToken& paramName)
{
    return _paramKnobCache.RegisterKnob(f, paramName);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_HYDRALIGHTOP_H
