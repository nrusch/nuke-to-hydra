#include <pxr/imaging/hd/tokens.h>

#include "instancerAdapter.h"


PXR_NAMESPACE_OPEN_SCOPE


void
HdNukeInstancerAdapter::Update(const GeoInfoVector& geoInfoPtrs)
{
    _instanceXforms.resize(geoInfoPtrs.size());
    for (size_t i = 0; i < geoInfoPtrs.size(); i++)
    {
        const float* matrixPtr = geoInfoPtrs[i]->matrix.array();
        std::copy(matrixPtr, matrixPtr + 16, _instanceXforms[i].data());
    }
}

VtValue
HdNukeInstancerAdapter::Get(const TfToken& key) const
{
    if (key == HdTokens->instanceTransform) {
        return VtValue(_instanceXforms);
    }
    return VtValue();
}


PXR_NAMESPACE_CLOSE_SCOPE
