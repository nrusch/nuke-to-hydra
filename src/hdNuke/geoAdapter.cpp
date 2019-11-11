#include <pxr/base/gf/vec2f.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/pxOsd/tokens.h>

#include "geoAdapter.h"
#include "tokens.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


HdNukeGeoAdapter::HdNukeGeoAdapter(const SdfPath& id,
                                   AdapterSharedState* statePtr,
                                   GeoInfo& geoInfo)
    : HdNukeAdapter(id, statePtr), _geo(&geoInfo)
{
    // TODO: Figure out Rprim type
}

GfMatrix4d
HdNukeGeoAdapter::GetTransform() const
{
    TF_VERIFY(_geo);
    return DDToGfMatrix4d(_geo->matrix);
}

HdMeshTopology
HdNukeGeoAdapter::GetMeshTopology() const
{
    TF_VERIFY(_geo);

    const uint32_t numPrimitives = _geo->primitives();

    size_t totalFaces = 0;
    size_t totalVerts = 0;
    const Primitive** primArray = _geo->primitive_array();
    for (size_t primIndex = 0; primIndex < numPrimitives; primIndex++)
    {
        totalFaces += primArray[primIndex]->faces();
        totalVerts += primArray[primIndex]->vertices();
    }

    VtIntArray faceVertexCounts;
    faceVertexCounts.reserve(totalFaces);
    VtIntArray faceVertexIndices;
    faceVertexIndices.reserve(totalVerts);

    // XXX: If any face has more than this many vertices, we're going to die.
    std::array<uint32_t, 16> faceVertices;

    for (uint32_t primIndex = 0; primIndex < numPrimitives; primIndex++)
    {
        const Primitive* prim = primArray[primIndex];
        const PrimitiveType primType = prim->getPrimitiveType();
        if (primType == ePoint or primType == eParticles
                or primType == eParticlesSprite) {
            // TODO: Warn/debug
            continue;
        }

        for (uint32_t faceIndex = 0; faceIndex < prim->faces(); faceIndex++)
        {
            const uint32_t numFaceVertices = prim->face_vertices(faceIndex);
            faceVertexCounts.push_back(numFaceVertices);

            prim->get_face_vertices(faceIndex, faceVertices.data());
            for (uint32_t faceVertexIndex = 0;
                 faceVertexIndex < numFaceVertices; faceVertexIndex++)
            {
                faceVertexIndices.push_back(
                    prim->vertex(faceVertices[faceVertexIndex]));
            }
        }
    }

    return HdMeshTopology(PxOsdOpenSubdivTokens->smooth,
                          UsdGeomTokens->rightHanded, faceVertexCounts,
                          faceVertexIndices);
}

VtValue
HdNukeGeoAdapter::Get(const TfToken& key) const
{
    TF_VERIFY(_geo);

// TODO: Attach node name as primvar
    if (key == HdTokens->points) {
        const PointList* pointList = _geo->point_list();
        if (ARCH_UNLIKELY(!pointList)) {
            return VtValue();
        }

        const auto* rawPoints = reinterpret_cast<const GfVec3f*>(pointList->data());
        VtVec3fArray ret;
        ret.assign(rawPoints, rawPoints + pointList->size());
        return VtValue::Take(ret);
    }
    else if (key == HdTokens->displayColor) {
        // TODO: Look up color from GeoInfo
        return VtValue(GetSharedState()->defaultDisplayColor);
    }

    TfToken attrName;
    if (key == HdNukeTokens->st) {
        attrName = HdNukeTokens->uv;
    }
    else if (key == HdTokens->normals) {
        attrName = HdNukeTokens->N;
    }
    else if (key == HdTokens->velocities) {
        attrName = HdNukeTokens->vel;
    }
    else {
        attrName = key;
    }

    const Attribute* geoAttr = _geo->get_attribute(attrName.GetText());
    if (ARCH_UNLIKELY(!geoAttr)) {
        TF_WARN("HdNukeGeoAdapter::Get : No geo attribute matches name %s",
                attrName.GetText());
        return VtValue();
    }
    if (!geoAttr->valid()) {
        TF_WARN("HdNukeGeoAdapter::Get : Invalid attribute: %s",
                attrName.GetText());
        return VtValue();
    }

    if (geoAttr->size() == 1) {
        void* rawData = geoAttr->array();
        float* floatData = static_cast<float*>(rawData);

        switch (geoAttr->type()) {
            case FLOAT_ATTRIB:
                return VtValue(floatData[0]);
            case INT_ATTRIB:
                return VtValue(static_cast<int32_t*>(rawData)[0]);
            case STRING_ATTRIB:
                return VtValue(std::string(static_cast<char**>(rawData)[0]));
            case STD_STRING_ATTRIB:
                return VtValue(static_cast<std::string*>(rawData)[0]);
            case VECTOR2_ATTRIB:
                return VtValue(GfVec2f(floatData));
            case VECTOR3_ATTRIB:
            case NORMAL_ATTRIB:
                return VtValue(GfVec3f(floatData));
            case VECTOR4_ATTRIB:
                return VtValue(GfVec4f(floatData));
            case MATRIX3_ATTRIB:
                {
                    GfMatrix3f gfMatrix;
                    std::copy(floatData, floatData + 9, gfMatrix.data());
                    return VtValue(gfMatrix);
                }
            case MATRIX4_ATTRIB:
                {
                    GfMatrix4f gfMatrix;
                    std::copy(floatData, floatData + 16, gfMatrix.data());
                    return VtValue(gfMatrix);
                }
        }
    }
    else {
        switch (geoAttr->type()) {
            case FLOAT_ATTRIB:
                return DDAttrToVtArrayValue<float>(geoAttr);
            case INT_ATTRIB:
                return DDAttrToVtArrayValue<int32_t>(geoAttr);
            case VECTOR2_ATTRIB:
                return DDAttrToVtArrayValue<GfVec2f>(geoAttr);
            case VECTOR3_ATTRIB:
            case NORMAL_ATTRIB:
                return DDAttrToVtArrayValue<GfVec3f>(geoAttr);
            case VECTOR4_ATTRIB:
                return DDAttrToVtArrayValue<GfVec4f>(geoAttr);
            case MATRIX3_ATTRIB:
                return DDAttrToVtArrayValue<GfMatrix3f>(geoAttr);
            case MATRIX4_ATTRIB:
                return DDAttrToVtArrayValue<GfMatrix4f>(geoAttr);
            case STD_STRING_ATTRIB:
                return DDAttrToVtArrayValue<std::string>(geoAttr);
            // XXX: Ignoring char* array attrs for now... not sure whether they
            // need special-case handling.
            // case STRING_ATTRIB:
        }
    }

    TF_WARN("HdNukeGeoAdapter::Get : Unhandled attribute type: %d",
            geoAttr->type());
    return VtValue();
}

HdPrimvarDescriptorVector
HdNukeGeoAdapter::GetPrimvarDescriptors(HdInterpolation interpolation) const
{
    // Group_Object      -> HdInterpolationConstant
    // Group_Primitives  -> HdInterpolationUniform
    // Group_Points (?)  -> HdInterpolationVertex
    // Group_Vertices    -> HdInterpolationFaceVarying

    TF_VERIFY(_geo);

    HdPrimvarDescriptorVector primvars;
    GroupType attrGroupType;

    switch (interpolation) {
        case HdInterpolationConstant:
            attrGroupType = Group_Object;
            primvars.push_back(
                HdPrimvarDescriptor(HdTokens->displayColor, interpolation,
                                    HdPrimvarRoleTokens->color));
            break;
        case HdInterpolationUniform:
            attrGroupType = Group_Primitives;
            break;
        case HdInterpolationVertex:
            attrGroupType = Group_Points;
            break;
        case HdInterpolationFaceVarying:
            attrGroupType = Group_Vertices;
            break;
        default:
            return primvars;
    }

    for (const auto& attribCtx : _geo->get_cache_pointer()->attributes)
    {
        if (attribCtx.group == attrGroupType and attribCtx.not_empty()) {
            TfToken attribName(attribCtx.name);
            TfToken role;
            if (attribName == HdNukeTokens->Cf) {
                role = HdPrimvarRoleTokens->color;
            }
            else if (attribName == HdNukeTokens->uv) {
                attribName = HdNukeTokens->st;
                role = HdPrimvarRoleTokens->textureCoordinate;
            }
            else if (attribName == HdNukeTokens->N) {
                attribName = HdTokens->normals;
                role = HdPrimvarRoleTokens->normal;
            }
            else if (attribName == HdNukeTokens->PW) {
                role = HdPrimvarRoleTokens->point;
            }
            else if (attribName == HdNukeTokens->vel) {
                attribName = HdTokens->velocities;
                role = HdPrimvarRoleTokens->vector;
            }
            else {
                role = HdPrimvarRoleTokens->none;
            }

            // Do we need to worry about AttribContext.varying?
            primvars.push_back(
                HdPrimvarDescriptor(attribName, interpolation, role));
        }
    }

    return primvars;
}


PXR_NAMESPACE_CLOSE_SCOPE
