#ifndef HDNUKE_MATERIALADAPTER_H
#define HDNUKE_MATERIALADAPTER_H

#include <map>

#include <pxr/pxr.h>


PXR_NAMESPACE_OPEN_SCOPE


class TfToken;
class VtValue;


class HdNukeMaterialAdapter
{
public:
    static VtValue GetPreviewMaterialResource(const SdfPath& materialId);

    static std::map<TfToken, VtValue> GetPreviewSurfaceParameters();

    static std::map<TfToken, VtValue> previewSurfaceParams;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_MATERIALADAPTER_H
