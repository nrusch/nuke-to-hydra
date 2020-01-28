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
