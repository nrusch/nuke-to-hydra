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
#include <pxr/imaging/hd/light.h>

#include <pxr/usd/usdLux/tokens.h>

#include <DDImage/Knobs.h>

#include "knobFactory.h"
#include "lightOp.h"
#include "primOpManager.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


void
HydraLightOp::knobs(Knob_Callback f)
{
    MakeLightKnobs(f);

    Divider(f);
    AxisOp::knobs(f);

    if (not (_knobCachePopulated or f.makeKnobs())) {
        _paramKnobCache.PopulateValues();
        _knobCachePopulated = true;
    }
}

int
HydraLightOp::knob_changed(Knob* k)
{
    if (k->is("translate") or k->is("rotate") or k->is("scaling")
            or k->is("uniform_scale") or k->is("skew") or k->is("pivot")
            or k->is("xform_order") or k->is("rot_order") or k->is("useMatrix"))
    {
        MarkDirty(HdLight::DirtyTransform);
        return 1;
    }

    if (k->is("shadow_enable")) {
        knob("shadow_color")->enable(_castShadows);
    }

    // This returns true if the knob is registered in the cache.
    if (_paramKnobCache.OnKnobChanged(k)) {
        MarkDirty(HdLight::DirtyParams);
        return 1;
    }

    return AxisOp::knob_changed(k);
}

void
HydraLightOp::Populate(HydraPrimOpManager* manager)
{
    manager->AddLight(this);
}

GfMatrix4d
HydraLightOp::GetTransform() const
{
    return DDToGfMatrix4d(matrix());
}

VtValue
HydraLightOp::GetLightParamValue(const TfToken& paramName)
{
    return _paramKnobCache.GetValue(paramName);
}

void
HydraLightOp::MakeLightKnobs(Knob_Callback f)
{
    Float_knob(f, &_intensity, "intensity");
    SetRange(f, 0, 5);
    RegisterLightParamKnob(f, UsdLuxTokens->intensity);

    Float_knob(f, &_exposure, "exposure");
    SetRange(f, -3, 3);
    RegisterLightParamKnob(f, UsdLuxTokens->exposure);

    Color_knob(f, _color, "color");
    RegisterLightParamKnob(f, UsdLuxTokens->color);

    Bool_knob(f, &_normalize, "normalize");
    SetFlags(f, Knob::STARTLINE);
    RegisterLightParamKnob(f, UsdLuxTokens->normalize);

    Float_knob(f, &_diffuse, "diffuse");
    RegisterLightParamKnob(f, UsdLuxTokens->diffuse);

    Float_knob(f, &_specular, "specular");
    RegisterLightParamKnob(f, UsdLuxTokens->specular);

    Bool_knob(f, &_castShadows, "cast_shadows", "cast shadows");
    SetFlags(f, Knob::STARTLINE);
    RegisterLightParamKnob(f, UsdLuxTokens->shadowEnable);

    Color_knob(f, _shadowColor, "shadow_color", "shadow color");
    RegisterLightParamKnob(f, UsdLuxTokens->shadowColor);
}


const HdDirtyBits HydraLightOp::DefaultDirtyBits =
    HdLight::DirtyTransform | HdLight::DirtyParams | HdLight::DirtyShadowParams;


PXR_NAMESPACE_CLOSE_SCOPE
