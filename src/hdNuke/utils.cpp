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
#include <pxr/usd/sdf/assetPath.h>

#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/Knobs.h>

#include "utils.h"


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


VtValue
KnobToVtValue(const Knob* knob)
{
    if (not knob) {
        return VtValue();
    }

    switch(knob->ClassID()) {
        case FLOAT_KNOB:
            return VtValue(static_cast<float>(knob->get_value()));
        case DOUBLE_KNOB:
            return VtValue(knob->get_value());
        case BOOL_KNOB:
            return VtValue(static_cast<bool>(knob->get_value()));
        case INT_KNOB:
            return VtValue(static_cast<int>(knob->get_value()));
        case ENUMERATION_KNOB:
        {
            const Enumeration_KnobI* enumKnob =
                const_cast<Knob*>(knob)->enumerationKnob();
            return VtValue(TfToken(enumKnob->getSelectedItemString()));
        }
        case COLOR_KNOB:
        case XYZ_KNOB:
            return VtValue(GfVec3f(knob->get_value(0), knob->get_value(1),
                                   knob->get_value(2)));
        case ACOLOR_KNOB:
            return VtValue(GfVec4f(knob->get_value(0), knob->get_value(1),
                                   knob->get_value(2), knob->get_value(3)));
        case STRING_KNOB:
            {
                const char* rawString = knob->get_text();
                return VtValue(std::string(rawString ? rawString : ""));
            }
        case FILE_KNOB:
            {
                const char* rawString = knob->get_text();
                const std::string path(rawString ? rawString : "");
                return VtValue(SdfAssetPath(path, path));
            }
        default:
            TF_WARN("KnobToVtValue : No VtValue conversion implemented for "
                    "knob type ID: %d", knob->ClassID());
    }
    return VtValue();
}


PXR_NAMESPACE_CLOSE_SCOPE
