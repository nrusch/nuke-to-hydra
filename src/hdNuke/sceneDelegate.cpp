#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/pxOsd/tokens.h>

#include "sceneDelegate.h"
#include "tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


HdNukeSceneDelegate::HdNukeSceneDelegate(HdRenderIndex* renderIndex)
    : HdSceneDelegate(renderIndex, DELEGATE_ID)
{

}

HdMeshTopology
HdNukeSceneDelegate::GetMeshTopology(const SdfPath& id)
{
    std::cerr << "HdNukeSceneDelegate::GetMeshTopology : " << id << std::endl;
    const GeoInfo* geoInfo = _rprimGeoInfos[id];
    TF_VERIFY(geoInfo);

    const uint32_t numPrimitives = geoInfo->primitives();

    size_t totalFaces = 0;
    const Primitive** primArray = geoInfo->primitive_array();
    for (size_t primIndex = 0; primIndex < numPrimitives; primIndex++)
    {
        totalFaces += primArray[primIndex]->faces();
    }

    VtIntArray faceVertexCounts;
    faceVertexCounts.reserve(totalFaces);
    VtIntArray faceVertexIndices;
    faceVertexIndices.reserve(static_cast<size_t>(geoInfo->vertices()));

    // XXX: If any face has more than this many vertices, we're going to die.
    std::array<uint32_t, 16> faceVertices;

    // TODO: Might want to build these in SyncFromGeoOp so we can verify that
    // the GeoInfo is a mesh?
    for (uint32_t primIndex = 0; primIndex < numPrimitives; primIndex++)
    {
        const Primitive* prim = primArray[primIndex];
        const PrimitiveType primType = prim->getPrimitiveType();
        if (primType == ePoint or primType == eParticles
                or primType == eParticlesSprite) {
            continue;
        }

        for (uint32_t faceIndex = 0; faceIndex < prim->faces(); faceIndex++)
        {
            const uint32_t numFaceVertices = prim->face_vertices(faceIndex);
            faceVertexCounts.push_back(numFaceVertices);

            prim->get_face_vertices(faceIndex, faceVertices.data());
            for (uint32_t faceVertexIndex = 0; faceVertexIndex < numFaceVertices; faceVertexIndex++)
            {
                faceVertexIndices.push_back(prim->vertex(faceVertices[faceVertexIndex]));
            }
        }
    }

    return HdMeshTopology(PxOsdOpenSubdivTokens->smooth,
                          UsdGeomTokens->rightHanded, faceVertexCounts,
                          faceVertexIndices);
}

GfMatrix4d
HdNukeSceneDelegate::GetTransform(const SdfPath& id)
{
    const GeoInfo* geoInfo = _rprimGeoInfos[id];
    TF_VERIFY(geoInfo);
    return DDToGfMatrix4d(geoInfo->matrix);
}

VtValue
HdNukeSceneDelegate::Get(const SdfPath& id, const TfToken& key)
{
    std::cerr << "HdNukeSceneDelegate::Get : " << id << ", " << key.GetString() << std::endl;

    const GeoInfo* geoInfo = _rprimGeoInfos[id];
    TF_VERIFY(geoInfo);

    if (key == HdTokens->points) {
        const PointList* pointList = geoInfo->point_list();
        if (ARCH_UNLIKELY(!pointList)) {
            return VtValue();
        }

        const auto* rawPoints = reinterpret_cast<const GfVec3f*>(pointList->data());
        VtVec3fArray ret;
        ret.assign(rawPoints, rawPoints + pointList->size());
        return VtValue::Take(ret);
    }

    const Attribute* geoAttr = geoInfo->get_attribute(key.GetText());
    if (ARCH_UNLIKELY(!geoAttr)) {
        TF_WARN("HdNukeSceneDelegate::Get : No geo attribute matches key %s",
            key.GetText());
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

    TF_WARN("HdNukeSceneDelegate::Get : Unhandled attribute type: %d",
            geoAttr->type());
    return VtValue();
}

HdPrimvarDescriptorVector
HdNukeSceneDelegate::GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation)
{
    // Group_Object      -> HdInterpolationConstant
    // Group_Primitives  -> HdInterpolationUniform
    // Group_Points (?)  -> HdInterpolationVarying
    // Group_Vertices    -> HdInterpolationVertex

    const GeoInfo* geoInfo = _rprimGeoInfos[id];
    TF_VERIFY(geoInfo);

    HdPrimvarDescriptorVector primvars;
    GroupType attrGroupType;

    switch (interpolation) {
        case HdInterpolationConstant:
            attrGroupType = Group_Object;
            break;
        case HdInterpolationUniform:
            attrGroupType = Group_Primitives;
            break;
        case HdInterpolationVarying:
            attrGroupType = Group_Points;
            break;
        case HdInterpolationVertex:
            attrGroupType = Group_Vertices;
            break;
        default:
            return primvars;
    }

    for (const auto& attribCtx : geoInfo->get_cache_pointer()->attributes)
    {
        if (attribCtx.group == attrGroupType and not attribCtx.empty()) {
            TfToken attribName(attribCtx.name);
            TfToken role;
            if (attribName == HdNukeTokens->Cf) {
                role = HdPrimvarRoleTokens->color;
            }
            else if (attribName == HdNukeTokens->uv) {
                role = HdPrimvarRoleTokens->textureCoordinate;
            }
            else if (attribName == HdNukeTokens->N) {
                role = HdPrimvarRoleTokens->normal;
            }
            else if (attribName == HdNukeTokens->PW) {
                role = HdPrimvarRoleTokens->point;
            }
            else {
                role = HdPrimvarRoleTokens->none;
            }

            primvars.push_back(
                HdPrimvarDescriptor(attribName, interpolation, role));
        }
    }

    return primvars;
}
SdfPath
HdNukeSceneDelegate::MakeRprimId(const GeoInfo& geoInfo) const
{
    std::ostringstream buf;
    buf << "src_" << std::hex << geoInfo.src_id().value();
    buf << "/out_" << geoInfo.out_id().value();
    return GEO_ROOT_ID.AppendPath(SdfPath(buf.str()));
}

void
HdNukeSceneDelegate::SyncFromGeoOp(GeoOp* op)
{
    TF_VERIFY(op);

    if (not op->valid()) {
        TF_CODING_ERROR("SyncFromGeoOp called with invalid GeoOp");
        return;
    }

    op->build_scene(_scene);
    GeometryList* geoList = _scene.object_list();

    std::cerr << "HdNukeSceneDelegate::SyncFromGeoOp" << std::endl;

    if (geoList->size() == 0) {
        Clear();
        return;
    }

    // Generate Rprim IDs for the GeoInfos in the scene
    RprimGeoInfoRefMap sceneGeoInfos;
    sceneGeoInfos.reserve(geoList->size());

    for (size_t i = 0; i < geoList->size(); i++)
    {
        const GeoInfo& geoInfo = geoList->object(i);

        SdfPath primId = MakeRprimId(geoInfo);
        sceneGeoInfos.emplace(primId, geoInfo);
    }

    HdRenderIndex& renderIndex = GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    // Insert new Rprims for IDs we don't already know about
    for (const auto& sceneGeoPair : sceneGeoInfos)
    {
        const SdfPath& primId = sceneGeoPair.first;
        if (_rprimGeoInfos.find(primId) == _rprimGeoInfos.end()) {
            _rprimGeoInfos[primId] = &sceneGeoPair.second;

            renderIndex.InsertRprim(HdPrimTypeTokens->mesh, this, primId);
        }
        else {
            // TODO: Support partially updating existing rprims. I think this
            // will require an awareness of src and out hashes for each rprim
            // (not necessarily in a hierarchy), and what to do if only one of
            // them changes. Thus, we will need to verify how modifying
            // different aspects of the GeoInfo affect those hashes. Probably
            // worth asking Foundry about this.
            // We may also need to rethink the prim IDs, particularly if the
            // output hash changes for things like transform, but the source
            // stays the same if the topology data is the same.
            // What about motion/deformation?

            changeTracker.MarkRprimDirty(primId);
        }
    }

    // Remove Rprims whose IDs are not in the new scene.
    for (auto it = _rprimGeoInfos.begin(); it != _rprimGeoInfos.end(); ) {
        if (sceneGeoInfos.find(it->first) == sceneGeoInfos.end()) {
            renderIndex.RemoveRprim(it->first);
            it = _rprimGeoInfos.erase(it);
        }
        else {
            it++;
        }
    }
}

void
HdNukeSceneDelegate::Clear()
{
    std::cerr << "HdNukeSceneDelegate::Clear" << std::endl;
    _rprimGeoInfos.clear();
    GetRenderIndex().RemoveSubtree(GEO_ROOT_ID, this);
}


PXR_NAMESPACE_CLOSE_SCOPE
