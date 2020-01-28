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

#include "lightAdapter.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


HdNukeLightAdapter::HdNukeLightAdapter(AdapterSharedState* statePtr,
                                       const LightOp* lightOp,
                                       const TfToken& lightType)
    : HdNukeAdapter(statePtr)
    , _light(lightOp)
    , _lightType(lightType)
    , _lastHash(lightOp->hash())
{
}

GfMatrix4d
HdNukeLightAdapter::GetTransform() const
{
    TF_VERIFY(_light);
    return DDToGfMatrix4d(_light->matrix());
}

VtValue
HdNukeLightAdapter::GetLightParamValue(const TfToken& paramName) const
{
// TODO: We need a way to make sure the light still exists in the scene...
    TF_VERIFY(_light);

    if (paramName == HdLightTokens->color) {
        auto& pixel = _light->color();
        return VtValue(
            GfVec3f(pixel[Chan_Red], pixel[Chan_Green], pixel[Chan_Blue]));
    }
    else if (paramName == HdLightTokens->intensity) {
        return VtValue(_light->intensity());
    }
    else if (paramName == HdLightTokens->radius) {
        return VtValue(_light->sample_width());
    }
    else if (paramName == HdLightTokens->shadowColor) {
        return VtValue(GfVec3f(0));
    }
    else if (paramName == HdLightTokens->shadowEnable) {
        return VtValue(_light->cast_shadows());
    }
    else if (paramName == HdLightTokens->exposure or
             paramName == HdLightTokens->diffuse or
             paramName == HdLightTokens->specular) {
        return VtValue(1.0f);
    }
    return VtValue();
}


HdDirtyBits HdNukeLightAdapter::DefaultDirtyBits =
    HdLight::DirtyTransform | HdLight::DirtyParams | HdLight::DirtyShadowParams;


PXR_NAMESPACE_CLOSE_SCOPE
