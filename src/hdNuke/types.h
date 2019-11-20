#ifndef HDNUKE_TYPES_H
#define HDNUKE_TYPES_H

#include <unordered_map>

#include <pxr/usd/sdf/path.h>

#include <DDImage/GeoInfo.h>


using namespace DD::Image;


namespace std
{
    template<>
    struct hash<Hash>
    {
        size_t operator()(const Hash& h) const
        {
            return h.value();
        }
    };
}  // namespace std


PXR_NAMESPACE_OPEN_SCOPE


using GeoOpHashArray = std::array<Hash, Group_Last>;
using GeoInfoVector = std::vector<const GeoInfo*>;

template <typename T>
using SdfPathMap = std::unordered_map<SdfPath, T, SdfPath::Hash>;


PXR_NAMESPACE_CLOSE_SCOPE


#endif  // HDNUKE_TYPES_H
