#include <pxr/base/gf/vec3f.h>

#include <pxr/imaging/hd/renderIndex.h>

#include <DDImage/NodeI.h>

#include "materialAdapter.h"
#include "sceneDelegate.h"
#include "tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


static SdfPath DELEGATE_ID("/Nuke");
static SdfPath GEO_ROOT = DELEGATE_ID.AppendChild(HdNukeTokens->Geo);
static SdfPath LIGHT_ROOT = DELEGATE_ID.AppendChild(HdNukeTokens->Lights);
static SdfPath MATERIAL_ROOT = DELEGATE_ID.AppendChild(HdNukeTokens->Materials);

static SdfPath DEFAULT_MATERIAL =
    MATERIAL_ROOT.AppendChild(HdNukeTokens->defaultSurface);


namespace
{
    inline SdfPath GetCleanOpPathTail(Op* op)
    {
        std::string tail(op->node_name());
        std::replace(tail.begin(), tail.end(), '.', '/');
        return SdfPath(tail);
    }
}  // namespace


HdNukeSceneDelegate::HdNukeSceneDelegate(HdRenderIndex* renderIndex)
    : HdSceneDelegate(renderIndex, DELEGATE_ID)
{

}

HdMeshTopology
HdNukeSceneDelegate::GetMeshTopology(const SdfPath& id)
{
    return GetGeoAdapter(id)->GetMeshTopology();
}

GfRange3d
HdNukeSceneDelegate::GetExtent(const SdfPath& id)
{
    return GetGeoAdapter(id)->GetExtent();
}

GfMatrix4d
HdNukeSceneDelegate::GetTransform(const SdfPath& id)
{
    if (id.HasPrefix(GEO_ROOT)) {
        return GetGeoAdapter(id)->GetTransform();
    }
    else if (id.HasPrefix(LIGHT_ROOT)) {
        return GetLightAdapter(id)->GetTransform();
    }

    TF_WARN("HdNukeSceneDelegate::GetTransform : Unrecognized prim id: %s",
        id.GetText());
    return GfMatrix4d(1);
}

bool
HdNukeSceneDelegate::GetVisible(const SdfPath& id)
{
    return GetGeoAdapter(id)->GetVisible();
}

VtValue
HdNukeSceneDelegate::Get(const SdfPath& id, const TfToken& key)
{
    return GetGeoAdapter(id)->Get(key);
}

SdfPath
HdNukeSceneDelegate::GetMaterialId(const SdfPath& rprimId)
{
    return DEFAULT_MATERIAL;
}

VtValue
HdNukeSceneDelegate::GetMaterialResource(const SdfPath& materialId)
{
    return HdNukeMaterialAdapter::GetPreviewMaterialResource(materialId);
}

HdPrimvarDescriptorVector
HdNukeSceneDelegate::GetPrimvarDescriptors(const SdfPath& id,
                                           HdInterpolation interpolation)
{
    return GetGeoAdapter(id)->GetPrimvarDescriptors(interpolation);
}

VtValue
HdNukeSceneDelegate::GetLightParamValue(const SdfPath& id,
                                        const TfToken& paramName)
{
    return GetLightAdapter(id)->GetLightParamValue(paramName);
}

HdNukeGeoAdapterPtr
HdNukeSceneDelegate::GetGeoAdapter(const SdfPath& id) const
{
    auto it = _geoAdapters.find(id);
    return it == _geoAdapters.end() ? nullptr : it->second;
}

HdNukeLightAdapterPtr
HdNukeSceneDelegate::GetLightAdapter(const SdfPath& id) const
{
    auto it = _lightAdapters.find(id);
    return it == _lightAdapters.end() ? nullptr : it->second;
}

TfToken
HdNukeSceneDelegate::GetRprimType(const GeoInfo& geoInfo) const
{
    const Primitive* firstPrim = geoInfo.primitive(0);
    if (firstPrim) {
        switch (firstPrim->getPrimitiveType()) {
            case eTriangle:
            case ePolygon:
            case eMesh:
            case ePolyMesh:
                return HdPrimTypeTokens->mesh;
            case eParticlesSprite:
                return HdPrimTypeTokens->points;
            // XXX: I have yet to encounter these, and I don't want to assume
            // they will be just like eParticlesSprite, so I'm just leaving some
            // canary warnings...
            case ePoint:
                TF_WARN("HdNukeSceneDelegate : Unhandled GeoInfo primitive "
                        "type : ePoint");
                break;
            case eParticles:
                TF_WARN("HdNukeSceneDelegate : Unhandled GeoInfo primitive "
                        "type : eParticles");
                break;
            default:
                break;
        }
    }
    return TfToken();
}

SdfPath
HdNukeSceneDelegate::GetRprimSubPath(const GeoInfo& geoInfo,
                                     const TfToken& primType) const
{
    if (primType.IsEmpty()) {
        return SdfPath();
    }

    // Look for an object-level "name" attribute on the geo, and if one is
    // found, use that as the prim's sub-path.
    const auto* nameCtx = geoInfo.get_group_attribcontext(Group_Object, "name");
    if (nameCtx and not nameCtx->empty()
            and (nameCtx->type == STRING_ATTRIB
                 or nameCtx->type == STD_STRING_ATTRIB))
    {
        void* rawData = nameCtx->attribute->array();
        std::string attrValue;
        if (nameCtx->type == STD_STRING_ATTRIB) {
            attrValue = static_cast<std::string*>(rawData)[0];
        }
        else {
            attrValue = std::string(static_cast<char**>(rawData)[0]);
        }

        SdfPath result(attrValue);
        if (result.IsAbsolutePath()) {
            return result.MakeRelativePath(SdfPath::AbsoluteRootPath());
        }
        return result;
    }

    std::ostringstream buf;
    buf << primType << '_' << std::hex << geoInfo.src_id().value();
    return SdfPath(buf.str());
}

uint32_t
HdNukeSceneDelegate::UpdateHashArray(const GeoOp* op, GeoOpHashArray& hashes) const
{
    uint32_t updateMask = 0;  // XXX: The mask enum in GeoInfo.h is untyped...
    for (uint32_t i = 0; i < Group_Last; i++)
    {
        const Hash groupHash(op->hash(i));
        if (groupHash != hashes[i]) {
            updateMask |= 1 << i;
        }
        hashes[i] = groupHash;
    }
    return updateMask;
}

void
HdNukeSceneDelegate::SetDefaultDisplayColor(GfVec3f color)
{
    if (color == sharedState.defaultDisplayColor) {
        return;
    }

    sharedState.defaultDisplayColor = color;
    if (not _geoAdapters.empty()) {
        HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
        for (auto it = _geoAdapters.cbegin(); it != _geoAdapters.cend(); it++) {
            tracker.MarkPrimvarDirty(it->first, HdTokens->displayColor);
        }
    }
}

void
HdNukeSceneDelegate::SyncGeometry(GeoOp* op, GeometryList* geoList)
{
    if (geoList->size() == 0) {
        ClearGeo();
        return;
    }

    std::unordered_map<GeoOp*, SdfPath> opSubtreeMap;
    std::unordered_map<GeoOp*, GeoInfoVector> opGeoMap;

    opSubtreeMap.reserve(op->inputs());  // Rough guess

    for (size_t i = 0; i < geoList->size(); i++)
    {
        // TODO: Check if all geos from an op are processed sequentially, since
        // that would allow us to optimize this.
        GeoInfo& geoInfo = geoList->object(i);
        GeoOp* sourceOp = op_cast<GeoOp*>(geoInfo.source_geo->firstOp());

        if (opSubtreeMap.find(sourceOp) == opSubtreeMap.end()) {
            opSubtreeMap.emplace(
                sourceOp, GEO_ROOT.AppendPath(GetCleanOpPathTail(sourceOp)));
        }

        opGeoMap[sourceOp].push_back(&geoInfo);
    }

    HdRenderIndex& renderIndex = GetRenderIndex();

    // Remove Rprim subtrees whose source GeoOps are not part of the new scene.
    for (auto it = _opSubtrees.begin(); it != _opSubtrees.end(); )
    {
        if (opSubtreeMap.find(it->first) == opSubtreeMap.end()) {
            std::cerr << "  RemoveSubtree : " << it->second << std::endl;

            renderIndex.RemoveSubtree(it->second, this);
            _opStateHashes.erase(it->first);
            it = _opSubtrees.erase(it);
            // TODO: Update other structures (maybe add a method)
        }
        else {
            it++;
        }
    }

    // Add prims from new ops and compute updates from existing ones
    for (const auto& opPathPair : opSubtreeMap)
    {
        GeoOp* sceneOp = opPathPair.first;
        const SdfPath& subtree = opPathPair.second;
        const GeoInfoVector& geoInfos = opGeoMap[sceneOp];

        if (_opSubtrees.find(sceneOp) == _opSubtrees.end()) {
            _opSubtrees.emplace(sceneOp, subtree);
            CreateOpGeo(sceneOp, subtree, geoInfos);
        }
        else {
            UpdateOpGeo(sceneOp, subtree, geoInfos);
        }
    }
}

void
HdNukeSceneDelegate::CreateOpGeo(GeoOp* geoOp, const SdfPath& subtree,
                                 const GeoInfoVector& geoInfos)
{
    GeoOpHashArray opHashes;
    UpdateHashArray(geoOp, opHashes);
    _opStateHashes.emplace(geoOp, std::move(opHashes));

    HdRenderIndex& renderIndex = GetRenderIndex();

    for (const GeoInfo* geoInfoPtr : geoInfos)
    {
        if (not geoInfoPtr) {
            continue;
        }
        const GeoInfo& geoInfo = *geoInfoPtr;

        TfToken primType = GetRprimType(geoInfo);
        if (primType.IsEmpty()
                or not renderIndex.IsRprimTypeSupported(primType)) {
            continue;
        }

        SdfPath subPath = GetRprimSubPath(geoInfo, primType);
        if (subPath.IsEmpty()) {
            continue;
        }

        // TODO: Collisions?
        SdfPath primId = subtree.AppendPath(subPath);
        if (primId == SdfPath::EmptyPath()) {
            continue;
        }

        auto adapterPtr = std::make_shared<HdNukeGeoAdapter>(&sharedState);
        _geoAdapters.emplace(primId, adapterPtr);

        // TODO: Check for collisions
        _geoInfoPrimIds.emplace(geoInfo.src_id(), primId);

        adapterPtr->Update(geoInfo);
        renderIndex.InsertRprim(primType, this, primId);
    }
}

void
HdNukeSceneDelegate::UpdateOpGeo(GeoOp* geoOp, const SdfPath& subtree,
                                 const GeoInfoVector& geoInfos)
{
    GeoOpHashArray& lastOpHashes = _opStateHashes[geoOp];
    auto updateMask = UpdateHashArray(geoOp, lastOpHashes);

    /*
    enum {
      Mask_No_Geometry  = 0x00000000,
      Mask_Primitives   = 0x00000001,  //!< Primitive list
      Mask_Vertices     = 0x00000002,  //!< Vertex group
      Mask_Points       = 0x00000004,  //!< Point list
      Mask_Object       = 0x00000008,  //!< The Object
      Mask_Matrix       = 0x00000010,  //!< Local->World Transform Matrix
      Mask_Attributes   = 0x00000020,  //!< Attribute list
    };
    */

    HdDirtyBits dirtyBits = HdChangeTracker::Clean;
    if (updateMask & Mask_Object) {
        // TODO: Need to do granular Rprim pruning in here
        // Check Mask_Object to decide whether we need to add/remove
        // (need to verify that this works as expected)

        // Mask_Object gets set for render mode changes as well
        dirtyBits |= HdChangeTracker::DirtyVisibility;
    }

    if (updateMask & (Mask_Primitives | Mask_Vertices)) {
        dirtyBits |= HdChangeTracker::DirtyTopology;
    }
    if (updateMask & Mask_Points) {
        dirtyBits |= (HdChangeTracker::DirtyPoints
                     | HdChangeTracker::DirtyExtent);
    }
    if (updateMask & Mask_Matrix) {
        dirtyBits |= HdChangeTracker::DirtyTransform;
    }
    if (updateMask & Mask_Attributes) {
        dirtyBits |= (HdChangeTracker::DirtyPrimvar
                      | HdChangeTracker::DirtyNormals
                      | HdChangeTracker::DirtyWidths);
    }

    if (dirtyBits == HdChangeTracker::Clean) {
        return;
    }

    HdChangeTracker& changeTracker = GetRenderIndex().GetChangeTracker();
    // TODO: geoInfos may be updated above...
    for (const GeoInfo* geoInfoPtr : geoInfos)
    {
        if (not geoInfoPtr) {
            continue;
        }
        const GeoInfo& geoInfo = *geoInfoPtr;

        SdfPath primId;
        const auto it = _geoInfoPrimIds.find(geoInfo.src_id());
        if (it != _geoInfoPrimIds.end()) {
            primId = it->second;
            if (not primId.HasPrefix(subtree)) {
                // TODO: Error
                TF_WARN("No hit for src_id in GeoInfo prim ID map");
                continue;
            }
        }
        else {
            SdfPath subPath = GetRprimSubPath(geoInfo, GetRprimType(geoInfo));
            if (subPath.IsEmpty()) {
                // Warn?
                continue;
            }
            primId = subtree.AppendPath(subPath);
        }

        auto adapterPtr = _geoAdapters[primId];
        adapterPtr->Update(geoInfo, dirtyBits);

        changeTracker.MarkRprimDirty(primId, dirtyBits);
    }
}

void
HdNukeSceneDelegate::SyncLights(std::vector<LightContext*> lights)
{
    SdfPathMap<LightOp*> sceneLights;

    for (const LightContext* lightCtx : lights)
    {
        LightOp* lightOp = dynamic_cast<LightOp*>(lightCtx->light()->firstOp());
        if (lightOp) {
            sceneLights.emplace(
                LIGHT_ROOT.AppendPath(GetCleanOpPathTail(lightOp)), lightOp);
        }
    }

    HdRenderIndex& renderIndex = GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    // Remove lights not in the new scene.
    for (auto it = _lightAdapters.begin(); it != _lightAdapters.end(); )
    {
        if (sceneLights.find(it->first) == sceneLights.end()) {
            renderIndex.RemoveSprim(it->second->GetLightType(), it->first);
            it = _lightAdapters.erase(it);
        }
        else {
            it++;
        }
    }

    for (const auto& lightInfo : sceneLights) {
        const SdfPath& lightId = lightInfo.first;
        LightOp* lightOp = lightInfo.second;

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

        if (not renderIndex.IsSprimTypeSupported(lightType)) {
            TF_WARN("Selected render delegate does not support Sprim type %s",
                    lightType.GetText());
            continue;
        }

        if (auto adapter = GetLightAdapter(lightId)) {
            if (adapter->GetLightType() == lightType) {
                if (adapter->DirtyHash()) {
                    changeTracker.MarkSprimDirty(
                        lightId, HdNukeLightAdapter::DefaultDirtyBits);
                    adapter->UpdateLastHash();
                }
                continue;
            }

            // Light ID is the same, but op type has changed, so blow it away.
            renderIndex.RemoveSprim(adapter->GetLightType(), lightId);
            _lightAdapters.erase(lightId);
        }

        auto adapterPtr = std::make_shared<HdNukeLightAdapter>(
            &sharedState, lightOp, lightType);
        _lightAdapters.emplace(lightId, adapterPtr);

        renderIndex.InsertSprim(lightType, this, lightId);
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

    HdRenderIndex& renderIndex = GetRenderIndex();

    if (renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->material)) {
        renderIndex.InsertSprim(
            HdPrimTypeTokens->material, this, DEFAULT_MATERIAL);
    }
}

void
HdNukeSceneDelegate::ClearAll()
{
    ClearGeo();
    ClearLights();
}

void
HdNukeSceneDelegate::ClearGeo()
{
    _geoAdapters.clear();
    _opSubtrees.clear();
    _opStateHashes.clear();
    _geoInfoPrimIds.clear();
    GetRenderIndex().RemoveSubtree(GEO_ROOT, this);
}

void
HdNukeSceneDelegate::ClearLights()
{
    _lightAdapters.clear();
    GetRenderIndex().RemoveSubtree(LIGHT_ROOT, this);
}


PXR_NAMESPACE_CLOSE_SCOPE
