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

#include <DDImage/Knobs.h>

#include "knobFactory.h"
#include "lightOp.h"
#include "primOpManager.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


void
HydraLightOp::knobs(Knob_Callback f)
{
    makeLightKnobs(f);

    Divider(f);
    AxisOp::knobs(f);
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
    MarkDirty(HdLight::DirtyParams);
    return 1;
}

void
HydraLightOp::makeLightKnobs(Knob_Callback f)
{
    Float_knob(f, &_intensity, "intensity");
    Float_knob(f, &_exposure, "exposure");
    SetRange(f, -3, 3);

    Color_knob(f, _color, "color");

    Bool_knob(f, &_normalize, "normalize");
    SetFlags(f, Knob::STARTLINE);

    Float_knob(f, &_diffuse, "diffuse");
    Float_knob(f, &_specular, "specular");

    Bool_knob(f, &_castShadows, "shadow_enable", "cast shadows");
    SetFlags(f, Knob::STARTLINE);
    Color_knob(f, _shadowColor, "shadow_color", "shadow color");
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
    std::string paramStr = paramName.GetString();
    std::replace(paramStr.begin(), paramStr.end(), ':', '_');

    if (Knob* k = knob(paramStr.c_str())) {
        return KnobToVtValue(k);
    }
    return VtValue();
}


const HdDirtyBits HydraLightOp::DefaultDirtyBits =
    HdLight::DirtyTransform | HdLight::DirtyParams | HdLight::DirtyShadowParams;


PXR_NAMESPACE_CLOSE_SCOPE
