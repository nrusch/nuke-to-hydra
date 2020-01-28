// Copyright 2019-present Nathan Rusch
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/pxOsd/tokens.h>

#include "geoAdapter.h"
#include "tokens.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


HdNukeGeoAdapter::HdNukeGeoAdapter(AdapterSharedState* statePtr)
    : HdNukeAdapter(statePtr)
{
}

void
HdNukeGeoAdapter::Update(const GeoInfo& geo, HdDirtyBits dirtyBits,
                         bool isInstanced)
{
    if (dirtyBits == HdChangeTracker::Clean) {
        return;
    }
    // XXX: For objects instanced by particle systems, Nuke includes the source
    // object's transform in the final transform of the instance. However,
    // because the render delegate will still query the scene delegate for the
    // attributes of the source Rprim (including transform), and then try to
    // concatenate them with the instance transform *itself*, we need to reset
    // the source transform here so it doesn't get applied twice.
    if (isInstanced) {
        _transform.SetIdentity();
    }
    else if (dirtyBits & HdChangeTracker::DirtyTransform) {
        _transform = DDToGfMatrix4d(geo.matrix);
    }

    if (dirtyBits & HdChangeTracker::DirtyVisibility) {
        _visible = geo.render_mode != RENDER_OFF;
    }

    if (dirtyBits & HdChangeTracker::DirtyTopology) {
        _RebuildMeshTopology(geo);
    }

    if (dirtyBits & HdChangeTracker::DirtyPoints) {
        _RebuildPointList(geo);
    }

    if (dirtyBits & (HdChangeTracker::DirtyPrimvar
                     | HdChangeTracker::DirtyNormals
                     | HdChangeTracker::DirtyWidths)) {
        _RebuildPrimvars(geo);
    }

    if (dirtyBits & HdChangeTracker::DirtyExtent) {
        const Vector3& min = geo.bbox().min();
        const Vector3& max = geo.bbox().max();
        _extent.SetMin(GfVec3d(min.x, min.y, min.z));
        _extent.SetMax(GfVec3d(max.x, max.y, max.z));
    }
}

HdPrimvarDescriptorVector
HdNukeGeoAdapter::GetPrimvarDescriptors(HdInterpolation interpolation) const
{
    switch (interpolation) {
        case HdInterpolationConstant:
            return _constantPrimvarDescriptors;
        case HdInterpolationUniform:
            return _uniformPrimvarDescriptors;
        case HdInterpolationVertex:
            return _vertexPrimvarDescriptors;
        case HdInterpolationFaceVarying:
            return _faceVaryingPrimvarDescriptors;
        default:
            return HdPrimvarDescriptorVector();
    }
}

void
HdNukeGeoAdapter::_RebuildMeshTopology(const GeoInfo& geo)
{
    const uint32_t numPrimitives = geo.primitives();

    size_t totalFaces = 0;
    size_t totalVerts = 0;
    const Primitive** primArray = geo.primitive_array();
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

    _topology = HdMeshTopology(PxOsdOpenSubdivTokens->smooth,
                               UsdGeomTokens->rightHanded, faceVertexCounts,
                               faceVertexIndices);
}

void HdNukeGeoAdapter::_RebuildPointList(const GeoInfo& geo)
{
    const PointList* pointList = geo.point_list();
    if (ARCH_UNLIKELY(!pointList)) {
        _points.clear();
        return;
    }

    const auto* rawPoints = reinterpret_cast<const GfVec3f*>(pointList->data());
    _points.assign(rawPoints, rawPoints + pointList->size());
}

VtValue
HdNukeGeoAdapter::Get(const TfToken& key) const
{
// TODO: Attach node name as primvar
    if (key == HdTokens->points) {
        return VtValue(_points);
    }
    else if (key == HdTokens->displayColor) {
        // TODO: Look up color from GeoInfo
        return VtValue(GetSharedState()->defaultDisplayColor);
    }
    else if (key == HdNukeTokens->st) {
        return VtValue(_uvs);
    }

    auto it = _primvarData.find(key);
    if (it != _primvarData.end()) {
        return it->second;
    }

    TF_WARN("HdNukeGeoAdapter::Get : Unrecognized key: %s", key.GetText());
    return VtValue();
}

void
HdNukeGeoAdapter::_RebuildPrimvars(const GeoInfo& geo)
{
    // Group_Object      -> HdInterpolationConstant
    // Group_Primitives  -> HdInterpolationUniform
    // Group_Points (?)  -> HdInterpolationVertex
    // Group_Vertices    -> HdInterpolationFaceVarying

    static HdPrimvarDescriptor displayColorDescriptor(
            HdTokens->displayColor, HdInterpolationConstant,
            HdPrimvarRoleTokens->color);

    // TODO: Try to reuse existing descriptors?
    _constantPrimvarDescriptors.clear();
    _constantPrimvarDescriptors.push_back(displayColorDescriptor);
    _uniformPrimvarDescriptors.clear();
    _vertexPrimvarDescriptors.clear();
    _faceVaryingPrimvarDescriptors.clear();

    _primvarData.clear();
    _primvarData.reserve(geo.get_attribcontext_count());

    for (const auto& attribCtx : geo.get_cache_pointer()->attributes)
    {
        if (attribCtx.empty()) {
            continue;
        }

        TfToken primvarName(attribCtx.name);
        TfToken role;

        if (primvarName == HdNukeTokens->Cf) {
            // attribName = HdTokens->faceColors;
            role = HdPrimvarRoleTokens->color;
        }
        else if (primvarName == HdNukeTokens->uv) {
            primvarName = HdNukeTokens->st;
            role = HdPrimvarRoleTokens->textureCoordinate;
        }
        else if (primvarName == HdNukeTokens->N) {
            primvarName = HdTokens->normals;
            role = HdPrimvarRoleTokens->normal;
        }
        else if (primvarName == HdNukeTokens->size) {
            primvarName = HdTokens->widths;
        }
        else if (primvarName == HdNukeTokens->PW) {
            role = HdPrimvarRoleTokens->point;
        }
        else if (primvarName == HdNukeTokens->vel) {
            primvarName = HdTokens->velocities;
            role = HdPrimvarRoleTokens->vector;
        }
        else {
            role = HdPrimvarRoleTokens->none;
        }

        switch (attribCtx.group) {
            case Group_Object:
                _constantPrimvarDescriptors.emplace_back(
                    primvarName, HdInterpolationConstant, role);
                break;
            case Group_Primitives:
                _uniformPrimvarDescriptors.emplace_back(
                    primvarName, HdInterpolationUniform, role);
                break;
            case Group_Points:
                _vertexPrimvarDescriptors.emplace_back(
                    primvarName, HdInterpolationVertex, role);
                break;
            case Group_Vertices:
                _faceVaryingPrimvarDescriptors.emplace_back(
                    primvarName, HdInterpolationFaceVarying, role);
                break;
            default:
                continue;
        }

        // Store attribute data
        const Attribute& attribute = *attribCtx.attribute;
        const AttribType attrType = attribute.type();

        // XXX: Special case for UVs. Nuke typically stores UVs as Vector4 (for
        // some inexplicable reason), but USD/Hydra conventions stipulate Vec2f.
        // Thus, we do type conversion in the case of a float vecter attr with
        // width > 2, just to be nice.
        if (primvarName == HdNukeTokens->st and (attrType == VECTOR4_ATTRIB
                                                 or attrType == VECTOR3_ATTRIB
                                                 or attrType == NORMAL_ATTRIB))
        {
            const auto size = attribute.size();
            _uvs.resize(size);
            float* dataPtr = static_cast<float*>(attribute.array());
            float* outPtr = reinterpret_cast<float*>(_uvs.data());
            const auto width = attribute.data_elements();
            for (size_t i = 0; i < size; i++, dataPtr += width) {
                *outPtr++ = dataPtr[0];
                *outPtr++ = dataPtr[1];
            }
            continue;
        }

        // General-purpose attribute conversions
        if (attribute.size() == 1) {
            void* rawData = attribute.array();
            float* floatData = static_cast<float*>(rawData);

            switch (attrType) {
                case FLOAT_ATTRIB:
                    _StorePrimvarScalar(primvarName, floatData[0]);
                    break;
                case INT_ATTRIB:
                    _StorePrimvarScalar(primvarName,
                                        static_cast<int32_t*>(rawData)[0]);
                    break;
                case STRING_ATTRIB:
                    _StorePrimvarScalar(primvarName,
                                        std::string(static_cast<char**>(rawData)[0]));
                    break;
                case STD_STRING_ATTRIB:
                    _StorePrimvarScalar(primvarName,
                                        static_cast<std::string*>(rawData)[0]);
                    break;
                case VECTOR2_ATTRIB:
                    _StorePrimvarScalar(primvarName, GfVec2f(floatData));
                    break;
                case VECTOR3_ATTRIB:
                case NORMAL_ATTRIB:
                    _StorePrimvarScalar(primvarName, GfVec3f(floatData));
                    break;
                case VECTOR4_ATTRIB:
                    _StorePrimvarScalar(primvarName, GfVec4f(floatData));
                    break;
                case MATRIX3_ATTRIB:
                    {
                        GfMatrix3f gfMatrix;
                        std::copy(floatData, floatData + 9, gfMatrix.data());
                        _StorePrimvarScalar(primvarName, gfMatrix);
                    }
                    break;
                case MATRIX4_ATTRIB:
                    {
                        GfMatrix4f gfMatrix;
                        std::copy(floatData, floatData + 16, gfMatrix.data());
                        _StorePrimvarScalar(primvarName, gfMatrix);
                    }
                    break;
                default:
                    TF_WARN("HdNukeGeoAdapter::_RebuildPrimvars : Unhandled "
                            "attribute type: %d", attrType);
                    continue;
            }
        }
        else {
            switch (attrType) {
                case FLOAT_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<float>(attribute));
                    break;
                case INT_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<int32_t>(attribute));
                    break;
                case VECTOR2_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<GfVec2f>(attribute));
                    break;
                case VECTOR3_ATTRIB:
                case NORMAL_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<GfVec3f>(attribute));
                    break;
                case VECTOR4_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<GfVec4f>(attribute));
                    break;
                case MATRIX3_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<GfMatrix3f>(attribute));
                    break;
                case MATRIX4_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<GfMatrix4f>(attribute));
                    break;
                case STD_STRING_ATTRIB:
                    _StorePrimvarArray(primvarName,
                                       DDAttrToVtArrayValue<std::string>(attribute));
                    break;
                default:
                    TF_WARN("HdNukeGeoAdapter::_RebuildPrimvars : Unhandled "
                            "attribute type: %d", attrType);
                    continue;

                // XXX: Ignoring char* array attrs for now... not sure whether they
                // need special-case handling.
                // case STRING_ATTRIB:
            }
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
