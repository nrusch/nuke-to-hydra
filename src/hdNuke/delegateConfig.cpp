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
#include "delegateConfig.h"
#include "tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


HdNukeDelegateConfig::HdNukeDelegateConfig()
    : _delegateId(DefaultDelegateID)
{
    _InitPaths();
}

HdNukeDelegateConfig::HdNukeDelegateConfig(const SdfPath& delegateId)
    : _delegateId(delegateId)
{
    _InitPaths();
}

void
HdNukeDelegateConfig::_InitPaths()
{
    _geoRoot = _delegateId.AppendChild(HdNukePathTokens->Geo);
    _lightRoot = _delegateId.AppendChild(HdNukePathTokens->Lights);
    _materialRoot = _delegateId.AppendChild(HdNukePathTokens->Materials);

    _nukeLightRoot = _lightRoot.AppendChild(HdNukePathTokens->Nuke);
    _hydraLightRoot = _lightRoot.AppendChild(HdNukePathTokens->Hydra);
}

const SdfPath HdNukeDelegateConfig::DefaultDelegateID("/HdNuke");


PXR_NAMESPACE_CLOSE_SCOPE
