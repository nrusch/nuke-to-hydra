#include "sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE


VtValue
HdNukeSceneDelegate::Get(SdfPath const& id, TfToken const& key)
{
    std::cerr << "HdNukeSceneDelegate::Get : " << id << ", " << key.GetString() << std::endl;
    return VtValue();
}


PXR_NAMESPACE_CLOSE_SCOPE
