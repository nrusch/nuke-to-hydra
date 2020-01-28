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
#include <pxr/base/vt/value.h>

#include <pxr/usd/sdr/registry.h>

#include <pxr/imaging/hd/material.h>

#include <pxr/usdImaging/usdImaging/tokens.h>

#include "materialAdapter.h"


PXR_NAMESPACE_OPEN_SCOPE


std::map<TfToken, VtValue> HdNukeMaterialAdapter::previewSurfaceParams;

/* static */
VtValue
HdNukeMaterialAdapter::GetPreviewMaterialResource(const SdfPath& materialId)
{
    HdMaterialNode node;
    node.identifier = UsdImagingTokens->UsdPreviewSurface;
    node.path = materialId.AppendChild(TfToken("Surface"));
    node.parameters = GetPreviewSurfaceParameters();

    HdMaterialNetwork network;
    network.nodes.push_back(node);

    HdMaterialNetworkMap map;
    map.map.emplace(HdMaterialTerminalTokens->surface, network);
    return VtValue::Take(map);
}

/* static */
std::map<TfToken, VtValue>
HdNukeMaterialAdapter::GetPreviewSurfaceParameters()
{
    static std::once_flag scanPreviewSurfaceParamsFlag;

    std::call_once(scanPreviewSurfaceParamsFlag, [] {
        auto& registry = SdrRegistry::GetInstance();
        SdrShaderNodeConstPtr sdrNode = registry.GetShaderNodeByIdentifier(
            UsdImagingTokens->UsdPreviewSurface);
        if (TF_VERIFY(sdrNode)) {
            for (const auto& inputName : sdrNode->GetInputNames())
            {
                auto shaderInput = sdrNode->GetInput(inputName);
                if (TF_VERIFY(shaderInput)) {
                    HdNukeMaterialAdapter::previewSurfaceParams.emplace(
                        inputName, shaderInput->GetDefaultValue());
                }
            }
        }
    });

    return HdNukeMaterialAdapter::previewSurfaceParams;
}

PXR_NAMESPACE_CLOSE_SCOPE
