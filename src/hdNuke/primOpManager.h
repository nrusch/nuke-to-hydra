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


class HydraPrimOpManager
{
public:
    void AddLight(HydraPrimOp* op);

    template<class OP_TYPE>
    SdfPath MakeGeometryId(OP_TYPE* op) const;

    template<class OP_TYPE>
    SdfPath MakeLightId(OP_TYPE* op) const;

    template<class OP_TYPE>
    SdfPath MakeMaterialId(OP_TYPE* op) const;

private:
    friend class HdNukeSceneDelegate;

    HydraPrimOpManager(HdNukeSceneDelegate* delegate)
        : _delegate(delegate) { }

    template <class OP_TYPE>
    bool _AddPrimOpToMap(OP_TYPE* op, const SdfPath& primId,
                         SdfPathMap<OP_TYPE*>& primOpMap);

    HdNukeSceneDelegate* _delegate;
};

template<class OP_TYPE>
SdfPath
HydraPrimOpManager::MakeGeometryId(OP_TYPE* op) const
{
    return _delegate->GetConfig().GeoRoot().AppendPath(GetPathFromOp(op));
}

template<class OP_TYPE>
SdfPath
HydraPrimOpManager::MakeLightId(OP_TYPE* op) const
{
    return _delegate->GetConfig().HydraLightRoot().AppendPath(GetPathFromOp(op));
}

template<class OP_TYPE>
SdfPath
HydraPrimOpManager::MakeMaterialId(OP_TYPE* op) const
{
    return _delegate->GetConfig().MaterialRoot().AppendPath(GetPathFromOp(op));
}

template <class OP_TYPE>
bool HydraPrimOpManager::_AddPrimOpToMap(OP_TYPE* op, const SdfPath& primId,
                                         SdfPathMap<OP_TYPE*>& primOpMap)
{
    TF_VERIFY(op);

    op = static_cast<OP_TYPE*>(op->firstOp());

    typename SdfPathMap<OP_TYPE*>::iterator it;
    bool inserted;
    std::tie(it, inserted) = primOpMap.insert(std::make_pair(primId, op));
    if (not inserted) {
        if (op != it->second) {
            // XXX: Should we just implicitly replace here?
            TF_CODING_ERROR("HydraPrimOpManager::_AddPrimOpToMap : Existing "
                            "prim ID %s maps to a different existing op. This "
                            "may do unexpected things...",
                            primId.GetText());
        }
    }
    return inserted;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDNUKE_OPMANAGER_H
