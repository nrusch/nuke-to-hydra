#ifndef HDNUKE_TYPES_H
#define HDNUKE_TYPES_H

#include <unordered_map>

#include <pxr/usd/sdf/path.h>

#include <DDImage/GeoInfo.h>


namespace std
{
    template<>
    struct hash<DD::Image::Hash>
    {
        size_t operator()(const DD::Image::Hash& h) const
        {
            return h.value();
        }
    };
}  // namespace std


PXR_NAMESPACE_OPEN_SCOPE


using GeoOpHashArray = std::array<DD::Image::Hash, DD::Image::Group_Last>;
using GeoInfoVector = std::vector<const DD::Image::GeoInfo*>;

template <typename T>
using SdfPathMap = std::unordered_map<SdfPath, T, SdfPath::Hash>;

template <typename T>
using TfTokenMap = std::unordered_map<TfToken, T, TfToken::HashFunctor>;


PXR_NAMESPACE_CLOSE_SCOPE


#endif  // HDNUKE_TYPES_H
