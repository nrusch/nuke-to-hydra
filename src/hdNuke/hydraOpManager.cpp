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

void
HydraOpManager::UpdateIndex(HydraOp* op)
{
    op->Populate(this);

    HdRenderIndex& renderIndex = _delegate->GetRenderIndex();

    const auto curMapEnd = _delegate->_hydraLightOps.end();
    const auto newMapEnd = _lightOps.end();
    for (auto it = _delegate->_hydraLightOps.begin(); it != curMapEnd; it++)
    {
        if (_lightOps.find(it->first) == newMapEnd) {
            renderIndex.RemoveSprim(it->second->GetPrimTypeName(), it->first);
        }
    }

    _lightOps.swap(_delegate->_hydraLightOps);
}


PXR_NAMESPACE_CLOSE_SCOPE
