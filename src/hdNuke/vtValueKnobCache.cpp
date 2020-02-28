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
#include <DDImage/Knobs.h>

#include "utils.h"
#include "vtValueKnobCache.h"


PXR_NAMESPACE_OPEN_SCOPE


bool
VtValueKnobCache::RegisterKnob(Knob* knob, const TfToken& key)
{
    if (not knob) {
        return false;
    }

    auto result = _knobKeyMap.emplace(knob, key);
    return result.second;
}

bool
VtValueKnobCache::RegisterKnob(Knob_Callback f, const TfToken& key)
{
    if (f.makeKnobs()) {
        Knob* k = f.getLastMadeKnob();
        if (k) {
            return RegisterKnob(k, key);
        }
    }
    return false;
}

void
VtValueKnobCache::PopulateValues(bool modifiedKnobsOnly)
{
    for (const auto& entry : _knobKeyMap)
    {
        if (modifiedKnobsOnly and not entry.first->not_default()) {
            continue;
        }
        _valueCache[entry.second] = std::move(KnobToVtValue(entry.first));
    }
}

bool
VtValueKnobCache::OnKnobChanged(Knob* knob)
{
    if (not knob) {
        return false;
    }
    const auto it = _knobKeyMap.find(knob);
    if (it == _knobKeyMap.end()) {
        return false;
    }
    _valueCache[it->second] = std::move(KnobToVtValue(knob));
    return true;
}

VtValue&
VtValueKnobCache::GetValue(const TfToken& key)
{
    return _valueCache[key];
}

void
VtValueKnobCache::Flush()
{
    _valueCache.clear();
}

void
VtValueKnobCache::Clear()
{
    _knobKeyMap.clear();
    _valueCache.clear();
}


PXR_NAMESPACE_CLOSE_SCOPE
