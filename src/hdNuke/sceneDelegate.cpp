#include <pxr/base/gf/vec3f.h>

#include <pxr/imaging/hd/renderIndex.h>

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
    return GetGeoAdapter(id)->GetMeshTopology();
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

VtValue
HdNukeSceneDelegate::Get(const SdfPath& id, const TfToken& key)
{
    std::cerr << "HdNukeSceneDelegate::Get : " << id << ", " << key.GetString() << std::endl;
    return GetGeoAdapter(id)->Get(key);
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
    auto result = it == _geoAdapters.end() ? nullptr : it->second;
    TF_VERIFY(result);
    return result;
}

HdNukeLightAdapterPtr
HdNukeSceneDelegate::GetLightAdapter(const SdfPath& id) const
{
    auto it = _lightAdapters.find(id);
    auto result = it == _lightAdapters.end() ? nullptr : it->second;
    TF_VERIFY(result);
    return result;
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
    if (color != sharedState.defaultDisplayColor) {
        sharedState.defaultDisplayColor = color;

        if (not _geoAdapters.empty()) {
            HdChangeTracker& tracker = GetRenderIndex().GetChangeTracker();
            for (const auto& adapterPair : _geoAdapters)
            {
                tracker.MarkPrimvarDirty(adapterPair.first, HdTokens->displayColor);
            }
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

    const bool anyDirtyBits = dirtyBits != HdChangeTracker::Clean;

    // Generate Rprim IDs for the GeoInfos in the scene
    SdfPathMap<GeoInfo&> sceneGeoInfos;
    sceneGeoInfos.reserve(geoList->size());

    for (size_t i = 0; i < geoList->size(); i++)
    {
        GeoInfo& geoInfo = geoList->object(i);
        sceneGeoInfos.emplace(MakeRprimId(geoInfo), geoInfo);
    }

    HdRenderIndex& renderIndex = GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    // Remove Rprims whose IDs are not in the new scene.
    for (auto it = _geoAdapters.begin(); it != _geoAdapters.end(); )
    {
        if (sceneGeoInfos.find(it->first) == sceneGeoInfos.end()) {
            renderIndex.RemoveRprim(it->first);
            it = _geoAdapters.erase(it);
        }
        else {
            it++;
        }
    }

    // Insert new Rprims and mark existing ones dirty
    for (const auto& sceneGeoPair : sceneGeoInfos)
    {
        const SdfPath& primId = sceneGeoPair.first;
        if (_geoAdapters.find(primId) == _geoAdapters.end()) {
            auto adapterPtr = std::make_shared<HdNukeGeoAdapter>(
                primId, &sharedState, sceneGeoPair.second);
            _geoAdapters[primId] = adapterPtr;

            renderIndex.InsertRprim(HdPrimTypeTokens->mesh, this, primId);
        }
        else {
            if (anyDirtyBits) {
                changeTracker.MarkRprimDirty(primId, dirtyBits);

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

        // TODO: Implement light dirtying
        if (_lightAdapters.find(lightId) != _lightAdapters.end()) {
            TF_CODING_ERROR("Skipping duplicate light ID: %s", lightId.GetText());
            continue;
        }

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

        if (renderIndex.IsSprimTypeSupported(lightType)) {
            auto adapterPtr = std::make_shared<HdNukeLightAdapter>(
                lightId, &sharedState, lightOp);
            _lightAdapters[lightId] = adapterPtr;

            renderIndex.InsertSprim(lightType, this, lightId);
        }
        else {
            TF_WARN("Selected render delegate does not support Sprim type %s",
                    lightType.GetText());
        }
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
    ClearLights();
}

void
HdNukeSceneDelegate::ClearGeo()
{
    _geoAdapters.clear();
    GetRenderIndex().RemoveSubtree(GEO_ROOT, this);
    for (auto& hash : _geoHashes) {
        hash.reset();
    }
}

void
HdNukeSceneDelegate::ClearLights()
{
    _lightAdapters.clear();
    GetRenderIndex().RemoveSubtree(LIGHT_ROOT, this);
}


PXR_NAMESPACE_CLOSE_SCOPE
