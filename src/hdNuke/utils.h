#ifndef HDNUKE_UTILS_H
#define HDNUKE_UTILS_H

#include <pxr/base/gf/matrix4d.h>

#include <DDImage/Matrix4.h>


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


inline GfMatrix4d DDToGfMatrix(const Matrix4& nukeMatrix) {
    GfMatrix4d gfMatrix;
    std::copy(nukeMatrix.array(), nukeMatrix.array() + 16, gfMatrix.data());
    return gfMatrix;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_UTILS_H
