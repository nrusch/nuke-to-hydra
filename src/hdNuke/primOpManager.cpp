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
#include "primOpManager.h"
#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE


void
HydraPrimOpManager::AddLight(HydraPrimOp* op)
{
    // TODO: Should we use a dynamic_cast as a guard? Or just trust that the
    // caller knows what they're doing?
    HydraLightOp* realOp = static_cast<HydraLightOp*>(op);

    SdfPath primId = MakeLightId(realOp);

    if (_AddPrimOpToMap(realOp, primId, _delegate->_hydraLightOps)) {
        _delegate->GetRenderIndex().InsertSprim(realOp->GetPrimTypeName(),
                                                _delegate, primId);
    }
    else {
        if (op->IsDirty()) {
            _delegate->GetRenderIndex().GetChangeTracker().MarkSprimDirty(
                primId, realOp->GetDirtyBits());
            op->MarkClean();
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
