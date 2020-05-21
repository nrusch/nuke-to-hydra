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
#include "lightOp.h"
#include "hydraOpManager.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


void
HydraOpManager::AddLight(HydraPrimOp* op)
{
    HydraLightOp* realOp = static_cast<HydraLightOp*>(op);

    SdfPath primId = MakeLightId(realOp);

    if (_InsertPrimOp(realOp, primId, _delegate->_hydraLightOps, _lightOps)) {
        _delegate->GetRenderIndex().InsertSprim(realOp->GetPrimTypeName(),
                                                _delegate, primId);
    }
    else {
        if (realOp->IsDirty()) {
            _delegate->GetRenderIndex().GetChangeTracker().MarkSprimDirty(
                primId, realOp->GetDirtyBits());
            realOp->MarkClean();
        }
    }
}

UsdImagingDelegate*
HydraOpManager::GetUsdDelegate(const SdfPath& delegateId)
{
    auto it = _delegate->_usdDelegates.find(delegateId);
    if (it != _delegate->_usdDelegates.end()) {
        _activeUsdDelegateIds.insert(delegateId);
        return it->second.get();
    }
    return nullptr;
}

UsdImagingDelegate*
HydraOpManager::CreateUsdDelegate(const SdfPath& delegateId)
{
    // Note that this currently unconditionally deletes any previously existing
    // delegate matching the given ID.
    UsdImagingDelegate* usdDelegate = new UsdImagingDelegate(
        &_delegate->GetRenderIndex(), delegateId);
    auto it = _delegate->_usdDelegates.find(delegateId);
    if (it != _delegate->_usdDelegates.end()) {
        it->second.reset(usdDelegate);
    }
    else {
        std::unique_ptr<UsdImagingDelegate> usdDelegatePtr(usdDelegate);
        _delegate->_usdDelegates.emplace(delegateId, std::move(usdDelegatePtr));
    }
    _activeUsdDelegateIds.insert(delegateId);
    return usdDelegate;
}

void
HydraOpManager::RemoveUsdDelegate(const SdfPath& delegateId)
{
    // The unique_ptr value type takes care of cleaning up the delegate, which
    // in turn removes its own subtree from the render index on deletion.
    _delegate->_usdDelegates.erase(delegateId);
    _activeUsdDelegateIds.erase(delegateId);
}

void
HydraOpManager::UpdateIndex(HydraOp* op)
{
    op->Populate(this);

    HdRenderIndex& renderIndex = _delegate->GetRenderIndex();

    // TODO: It would be nice to have a more pluggable way of handling pruning.

    // Prune prims for disconnected light ops
    const auto curLightMapEnd = _delegate->_hydraLightOps.end();
    const auto newLightMapEnd = _lightOps.end();
    for (auto it = _delegate->_hydraLightOps.begin(); it != curLightMapEnd; it++)
    {
        if (_lightOps.find(it->first) == newLightMapEnd) {
            renderIndex.RemoveSprim(it->second->GetPrimTypeName(), it->first);
        }
    }

    _lightOps.swap(_delegate->_hydraLightOps);

    // Prune UsdImagingDelegates for disconnected stage ops
    const auto curDelegateMapEnd = _delegate->_usdDelegates.end();
    const auto activeDelegateIdsEnd = _activeUsdDelegateIds.end();
    for (auto it = _delegate->_usdDelegates.begin(); it != curDelegateMapEnd; )
    {
        if (_activeUsdDelegateIds.find(it->first) == activeDelegateIdsEnd) {
            it = _delegate->_usdDelegates.erase(it);
        }
        else {
            it++;
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
