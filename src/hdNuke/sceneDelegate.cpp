#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/pxOsd/tokens.h>

#include <DDImage/NodeI.h>

#include "sceneDelegate.h"
#include "tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


static SdfPath DELEGATE_ID(TfToken("/Nuke", TfToken::Immortal));
static SdfPath GEO_ROOT = DELEGATE_ID.AppendChild(TfToken("Geo"));
static SdfPath LIGHT_ROOT = DELEGATE_ID.AppendChild(TfToken("Lights"));


HdNukeSceneDelegate::HdNukeSceneDelegate(HdRenderIndex* renderIndex)
    : HdSceneDelegate(renderIndex, DELEGATE_ID)
{

}

HdMeshTopology
HdNukeSceneDelegate::GetMeshTopology(const SdfPath& id)
{
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
    if (id.HasPrefix(GEO_ROOT)) {
        const GeoInfo* geoInfo = _rprimGeoInfos[id];
        TF_VERIFY(geoInfo);
        return DDToGfMatrix4d(geoInfo->matrix);
    }
    else if (id.HasPrefix(LIGHT_ROOT)) {
        const LightOp* light = _lightOps[id];
        TF_VERIFY(light);
        return DDToGfMatrix4d(light->matrix());
    }

    TF_WARN("HdNukeSceneDelegate::GetTransform : Unrecognized prim id: %s",
        id.GetText());
    return GfMatrix4d(1);
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
    else if (key == HdTokens->displayColor) {
        return VtValue(_defaultDisplayColor);
    }

    const Attribute* geoAttr = geoInfo->get_attribute(key.GetText());
    if (ARCH_UNLIKELY(!geoAttr)) {
        TF_WARN("HdNukeSceneDelegate::Get : No geo attribute matches key %s",
            key.GetText());
        return VtValue();
    }
    if (!geoAttr->valid()) {
        TF_WARN("HdNukeSceneDelegate::Get : Invalid attribute: %s",
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
            primvars.push_back(
                HdPrimvarDescriptor(HdTokens->displayColor, interpolation,
                                    HdPrimvarRoleTokens->color));
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
    return GEO_ROOT.AppendPath(SdfPath(buf.str()));
}

void
HdNukeSceneDelegate::SetDefaultDisplayColor(GfVec3f color)
{
    _defaultDisplayColor = color;

    if (not _rprimGeoInfos.empty()) {
        HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
        for (const auto& rprimPair : _rprimGeoInfos)
        {
            tracker.MarkPrimvarDirty(rprimPair.first, HdTokens->displayColor);
        }
    }
}

void
HdNukeSceneDelegate::SyncGeometry(GeoOp* op, GeometryList* geoList)
{
    std::cerr << "  GeoOp rebuild_mask: " << op->rebuild_mask() << std::endl;

    // Compute the dirty bits for existing Rprims from the op's geometry hashes
    std::array<Hash, Group_Last> opGeoHashes;
    uint32_t updateMask = 0;  // XXX: The mask enum in GeoInfo.h is untyped...

    for (uint32_t i = 0; i < Group_Last; i++)
    {
        const Hash groupHash(op->hash(i));
        opGeoHashes[i] = groupHash;
        if (groupHash != _geoHashes[i]) {
            updateMask |= 1 << i;
        }
    }

    // Replace stored hashes
    opGeoHashes.swap(_geoHashes);

    if (geoList->size() == 0) {
        ClearGeo();
        return;
    }

    HdDirtyBits dirtyBits = HdChangeTracker::Clean;
    if (updateMask & (Mask_Primitives | Mask_Vertices)) {
        dirtyBits |= (HdChangeTracker::DirtyTopology
                      | HdChangeTracker::DirtyNormals
                      | HdChangeTracker::DirtyWidths);
    }
    if (updateMask & Mask_Points) {
        dirtyBits |= HdChangeTracker::DirtyPoints;
    }

    if (updateMask & Mask_Matrix) {
        dirtyBits |= HdChangeTracker::DirtyTransform;
    }

    if (updateMask & Mask_Matrix) {
        dirtyBits |= HdChangeTracker::DirtyTransform;
    }

    if (updateMask & Mask_Attributes) {
        dirtyBits |= HdChangeTracker::DirtyPrimvar;
    }

    bool anyDirtyBits = dirtyBits != HdChangeTracker::Clean;

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
            if (anyDirtyBits) {
                changeTracker.MarkRprimDirty(primId, dirtyBits);

            }
        }
    }

    // Remove Rprims whose IDs are not in the new scene.
    for (auto it = _rprimGeoInfos.begin(); it != _rprimGeoInfos.end(); )
    {
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
HdNukeSceneDelegate::SyncLights(std::vector<LightContext*> lights)
{
    HdRenderIndex& renderIndex = GetRenderIndex();

    for (const LightContext* lightCtx : lights)
    {
        LightOp* lightOp = lightCtx->light();

        SdfPath lightId = LIGHT_ROOT.AppendChild(
            TfToken(lightOp->getNode()->getNodeName()));

        TfToken lightType;
        switch (lightOp->lightType()) {
            case LightOp::eDirectionalLight:
                lightType = HdPrimTypeTokens->distantLight;
                break;
            case LightOp::eSpotLight:
                lightType = HdPrimTypeTokens->diskLight;
                break;
            case LightOp::ePointLight:
                lightType = HdPrimTypeTokens->sphereLight;
                break;
            case LightOp::eOtherLight:
                // XXX: The only other current type is an environment light...
                lightType = HdPrimTypeTokens->domeLight;
                break;
            default:
                continue;
        }

        renderIndex.InsertSprim(lightType, this, lightId);
        _lightOps[lightId] = lightOp;
    }
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

    SyncGeometry(op, geoList);

    SyncLights(_scene.lights);
}

void
HdNukeSceneDelegate::ClearAll()
{

    ClearGeo();
}

void
HdNukeSceneDelegate::ClearGeo()
{
    _rprimGeoInfos.clear();
    GetRenderIndex().RemoveSubtree(GEO_ROOT, this);
    for (auto& hash : _geoHashes) {
        hash.reset();
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
