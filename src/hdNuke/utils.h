#ifndef HDNUKE_UTILS_H
#define HDNUKE_UTILS_H

#include <pxr/base/gf/matrix3f.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/matrix4d.h>

#include <DDImage/Matrix4.h>


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


inline GfMatrix3f DDToGfMatrix3f(const Matrix3& nukeMatrix)
{
    GfMatrix3f gfMatrix;
    std::copy(nukeMatrix.array(), nukeMatrix.array() + 9, gfMatrix.data());
    return gfMatrix;
}

inline GfMatrix4f DDToGfMatrix4f(const Matrix4& nukeMatrix)
{
    GfMatrix4f gfMatrix;
    std::copy(nukeMatrix.array(), nukeMatrix.array() + 16, gfMatrix.data());
    return gfMatrix;
}

inline GfMatrix4d DDToGfMatrix4d(const Matrix4& nukeMatrix)
{
    GfMatrix4d gfMatrix;
    std::copy(nukeMatrix.array(), nukeMatrix.array() + 16, gfMatrix.data());
    return gfMatrix;
}

template <typename T>
inline VtValue
DDAttrToVtArrayValue(const Attribute* geoAttr)
{
    // XXX: Would be interesting to try using `Vt_ArrayForeignDataSource`...
    VtArray<T> array;
    auto* dataVec = static_cast<std::vector<T>*>(geoAttr->array());
    array.assign(dataVec->begin(), dataVec->end());
    return VtValue::Take(array);
}

template <typename T>
inline void
convertRenderBufferData(void* rawData, float* dest, size_t numPixels,
                        size_t numComponents, bool packed)
{
    const T* data;
    if (packed or numComponents == 1) {
        data = static_cast<T*>(rawData);
        for (size_t i = 0; i < numPixels * numComponents; i++)
        {
            dest[i] = static_cast<float>(data[i]);
        }
    }
    else {
        for (size_t chan = 0; chan < numComponents; chan++)
        {
            data = static_cast<T*>(rawData) + chan;
            for (size_t i = 0; i < numPixels; i++)
            {
                *dest++ = static_cast<float>(data[i * numComponents]);
            }
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_UTILS_H
