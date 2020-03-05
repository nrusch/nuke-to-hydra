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
#ifndef HDNUKE_OPMANAGER_H
#define HDNUKE_OPMANAGER_H

#include <pxr/pxr.h>

#include "opBases.h"
#include "sceneDelegate.h"


PXR_NAMESPACE_OPEN_SCOPE


class HydraOpManager
{
public:
    void AddLight(HydraPrimOp* op);

private:
    friend class HdNukeSceneDelegate;

    HydraOpManager(HdNukeSceneDelegate* delegate) : _delegate(delegate) { }

    void UpdateIndex(HydraOp* op);

    template<class OP_TYPE>
    SdfPath MakeGeometryId(OP_TYPE* op) const;

    template<class OP_TYPE>
    SdfPath MakeLightId(OP_TYPE* op) const;

    template<class OP_TYPE>
    SdfPath MakeMaterialId(OP_TYPE* op) const;

    template <class OP_TYPE>
    bool _InsertPrimOp(OP_TYPE* op, const SdfPath& primId,
                       const SdfPathMap<OP_TYPE*>& currentOpMap,
                       SdfPathMap<OP_TYPE*>& newOpMap);

    HdNukeSceneDelegate* _delegate;

    SdfPathMap<HydraLightOp*> _lightOps;
};


template<class OP_TYPE>
SdfPath
HydraOpManager::MakeGeometryId(OP_TYPE* op) const
{
    return _delegate->GetConfig().GeoRoot().AppendPath(GetPathFromOp(op));
}

template<class OP_TYPE>
SdfPath
HydraOpManager::MakeLightId(OP_TYPE* op) const
{
    return _delegate->GetConfig().HydraLightRoot().AppendPath(GetPathFromOp(op));
}

template<class OP_TYPE>
SdfPath
HydraOpManager::MakeMaterialId(OP_TYPE* op) const
{
    return _delegate->GetConfig().MaterialRoot().AppendPath(GetPathFromOp(op));
}

template <class OP_TYPE>
bool HydraOpManager::_InsertPrimOp(OP_TYPE* op, const SdfPath& primId,
                                   const SdfPathMap<OP_TYPE*>& currentOpMap,
                                   SdfPathMap<OP_TYPE*>& newOpMap)
{
    TF_VERIFY(op);

    op = static_cast<OP_TYPE*>(op->firstOp());

    auto insertResult = newOpMap.insert(std::make_pair(primId, op));
    if (ARCH_UNLIKELY(not insertResult.second)) {
        TF_CODING_ERROR("HydraOpManager::_InsertPrimOp : More than one op maps "
                        "to prim ID %s - ignoring duplicates",
                        primId.GetText());
        return false;
    }

    const auto it = currentOpMap.find(primId);
    const bool isNewOp = it == currentOpMap.end();
    if (not isNewOp) {
        const TfToken& existingType = it->second->GetPrimTypeName();
        const TfToken& newType = op->GetPrimTypeName();
        if (newType != existingType) {
            TF_CODING_ERROR("HydraOpManager::_InsertPrimOp : Prim types of new "
                            "new and existing ops for prim ID %s do not match: "
                            "%s != %s",
                            primId.GetText(), newType.GetText(),
                            existingType.GetText());
        }
    }
    return isNewOp;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDNUKE_OPMANAGER_H
