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
    if (id.GetName() == HdTokens->instancer) {
        return GetInstancerAdapter(id)->Get(key);
    }
    return GetGeoAdapter(id)->Get(key);
}

VtIntArray
HdNukeSceneDelegate::GetInstanceIndices(const SdfPath& instancerId,
                                        const SdfPath& prototypeId)
{
    auto adapter = GetInstancerAdapter(instancerId);
    VtIntArray result(adapter->InstanceCount());
    std::iota(result.begin(), result.end(), 0);
    return result;
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
    if (interpolation == HdInterpolationInstance) {
        HdPrimvarDescriptorVector primvars;
        primvars.emplace_back(HdTokens->instanceTransform, interpolation);
        return primvars;
    }
    else if (id.HasPrefix(GEO_ROOT)) {
        return GetGeoAdapter(id)->GetPrimvarDescriptors(interpolation);
    }
    return HdPrimvarDescriptorVector();
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

HdNukeInstancerAdapterPtr
HdNukeSceneDelegate::GetInstancerAdapter(const SdfPath& id) const
{
    auto it = _instancerAdapters.find(id);
    return it == _instancerAdapters.end() ? nullptr : it->second;
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
HdNukeSceneDelegate::SyncGeometry(GeometryList* geoList)
{
    if (geoList->size() == 0) {
        ClearGeo();
        return;
    }

    std::unordered_map<GeoOp*, SdfPath> opSubtreeMap;
    std::unordered_map<GeoOp*, std::unordered_map<Hash, GeoInfoVector>> geoSourceMap;

    for (size_t i = 0; i < geoList->size(); i++)
    {
        GeoInfo& geoInfo = geoList->object(i);
        GeoOp* sourceOp = op_cast<GeoOp*>(geoInfo.source_geo->firstOp());

        if (opSubtreeMap.find(sourceOp) == opSubtreeMap.end()) {
            opSubtreeMap.emplace(
                sourceOp, GEO_ROOT.AppendPath(GetCleanOpPathTail(sourceOp)));
        }

        geoSourceMap[sourceOp][geoInfo.src_id()].push_back(&geoInfo);
    }

    HdRenderIndex& renderIndex = GetRenderIndex();

    // Remove Rprim subtrees whose source GeoOps are not part of the new scene.
    for (auto it = _opSubtrees.begin(); it != _opSubtrees.end(); )
    {
        if (opSubtreeMap.find(it->first) == opSubtreeMap.end()) {
            _RemoveSubtree(it->second);
            _opStateHashes.erase(it->first);
            it = _opSubtrees.erase(it);
        }
        else {
            it++;
        }
    }

    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    for (const auto& geoSourceMapEntry : geoSourceMap)
    {
        GeoOp* sourceOp = geoSourceMapEntry.first;
        const SdfPath& subtree = opSubtreeMap[sourceOp];

        HdDirtyBits opDirtyBits = HdChangeTracker::AllDirty;
        bool newOp = false;

        auto opHashIter = _opStateHashes.find(sourceOp);
        if (opHashIter == _opStateHashes.end()) {
            GeoOpHashArray opHashes;
            UpdateHashArray(sourceOp, opHashes);
            _opSubtrees.emplace(sourceOp, subtree);
            _opStateHashes.emplace(sourceOp, std::move(opHashes));
            newOp = true;
        }
        else {
            // Compute update mask
            // TODO: Double-check that this works
            uint32_t updateMask = UpdateHashArray(sourceOp, opHashIter->second);
            opDirtyBits = DirtyBitsFromUpdateMask(updateMask);
        }

        std::unordered_set<SdfPath, SdfPath::Hash> existingPrimIds;
        SdfPathVector lastOpRprimSubtree = renderIndex.GetRprimSubtree(subtree);

        for (const auto& geoInfoIdEntry : geoSourceMapEntry.second)
        {
            // Hash& geoSrcId = geoInfoIdEntry.first;

            const GeoInfoVector& geoInfos = geoInfoIdEntry.second;
            const GeoInfo& firstGeo = *geoInfos[0];

            TfToken primType = GetRprimType(firstGeo);
            if (primType.IsEmpty()
                    or not renderIndex.IsRprimTypeSupported(primType)) {
                continue;
            }

            SdfPath subPath = GetRprimSubPath(firstGeo, primType);
            if (subPath.IsEmpty()) {
                continue;
            }

            SdfPath primId = subtree.AppendPath(subPath);
            if (primId.IsEmpty()) {
                continue;
            }

            SdfPath instancerId = primId.AppendChild(HdTokens->instancer);
            HdNukeInstancerAdapterPtr instAdapter = GetInstancerAdapter(instancerId);
            bool createdNewInstancer = false;

            if (geoInfos.size() > 1) {
                if (not instAdapter) {
                    instAdapter = std::make_shared<HdNukeInstancerAdapter>(
                        &sharedState);
                    _instancerAdapters.emplace(instancerId, instAdapter);
                    renderIndex.InsertInstancer(this, instancerId);
                    createdNewInstancer = true;
                }

            }

            // XXX: If there's an existing instancer but only 1 instance, we
            // leave the instancer in place, just to simplify the bookkeeping.

            if (instAdapter) {
                instAdapter->Update(geoInfos);
            }

            HdNukeGeoAdapterPtr geoAdapter = GetGeoAdapter(primId);
            bool needNewPrim = false;

            if (createdNewInstancer and geoAdapter != nullptr) {
                // Need to re-insert Rprim to create instancer relationship
                renderIndex.RemoveRprim(primId);
                needNewPrim = true;
            }
            else if (geoAdapter == nullptr) {
                geoAdapter = std::make_shared<HdNukeGeoAdapter>(&sharedState);
                _geoAdapters.emplace(primId, geoAdapter);

                needNewPrim = true;
            }

            HdDirtyBits geoDirtyBits;

            if (needNewPrim) {
                if (not instAdapter) {
                    instancerId = SdfPath();
                }
                renderIndex.InsertRprim(primType, this, primId, instancerId);
                geoDirtyBits = HdChangeTracker::AllDirty;
            }
            else {
                geoDirtyBits = opDirtyBits;
                if (geoDirtyBits != HdChangeTracker::Clean) {
                    changeTracker.MarkRprimDirty(primId, geoDirtyBits);
                }
            }

            geoAdapter->Update(firstGeo, geoDirtyBits,
                               static_cast<bool>(instAdapter));

            if (instAdapter and not createdNewInstancer) {
                changeTracker.MarkInstancerDirty(instancerId);
            }

            // Minor optimization
            if (not newOp) {
                existingPrimIds.insert(primId);
            }
        }

        if (not newOp) {
            for (const auto& indexPrimId : lastOpRprimSubtree)
            {
                if (existingPrimIds.find(indexPrimId) == existingPrimIds.end()) {
                    _RemoveRprim(indexPrimId);
                }
            }
        }
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

    SyncGeometry(_scene.object_list());
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
    _instancerAdapters.clear();
    _opSubtrees.clear();
    _opStateHashes.clear();
    GetRenderIndex().RemoveSubtree(GEO_ROOT, this);
}

void
HdNukeSceneDelegate::ClearLights()
{
    _lightAdapters.clear();
    GetRenderIndex().RemoveSubtree(LIGHT_ROOT, this);
}

inline void
HdNukeSceneDelegate::_RemoveRprim(const SdfPath& primId)
{
    HdRenderIndex& renderIndex = GetRenderIndex();
    renderIndex.RemoveSubtree(primId, this);
    _geoAdapters.erase(primId);
    _instancerAdapters.erase(primId.AppendChild(HdTokens->instancer));
}

void
HdNukeSceneDelegate::_RemoveSubtree(const SdfPath& subtree)
{
    for (auto adapterIt = _geoAdapters.begin(); adapterIt != _geoAdapters.end();)
    {
        const SdfPath& rprimPath = adapterIt->first;
        if (rprimPath.HasPrefix(subtree)) {
            _instancerAdapters.erase(rprimPath.AppendChild(HdTokens->instancer));
            adapterIt = _geoAdapters.erase(adapterIt);
        }
        else {
            adapterIt++;
        }
    }

    GetRenderIndex().RemoveSubtree(subtree, this);
}

/* static */
uint32_t
HdNukeSceneDelegate::UpdateHashArray(const GeoOp* op, GeoOpHashArray& hashes)
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

/* static */
HdDirtyBits
HdNukeSceneDelegate::DirtyBitsFromUpdateMask(uint32_t updateMask)
{
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
    return dirtyBits;
}


PXR_NAMESPACE_CLOSE_SCOPE
