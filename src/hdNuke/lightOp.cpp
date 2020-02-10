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
#include <DDImage/Knobs.h>

#include "lightOp.h"
#include "primOpManager.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


void
HydraLightOp::knobs(Knob_Callback f)
{
    Float_knob(f, &_intensity, "intensity");
    Float_knob(f, &_exposure, "exposure");

    Color_knob(f, _color, "color");

    Bool_knob(f, &_normalize, "normalize");

    Float_knob(f, &_diffuse, "diffuse");
    Float_knob(f, &_specular, "specular");

    Bool_knob(f, &_castShadows, "cast_shadows", "cast shadows");
    Color_knob(f, _shadowColor, "shadow_color", "shadow color");
}

int
HydraLightOp::knob_changed(Knob* k)
{
    if (k->is("cast_shadows")) {
        knob("shadow_color")->enable(_castShadows);
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
    std::cerr << "GetLightParamValue : " << node_name()
        << " : " << paramName.GetString() << std::endl;
    return VtValue();
}


PXR_NAMESPACE_CLOSE_SCOPE
