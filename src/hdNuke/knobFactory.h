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
#ifndef HDNUKE_KNOBFACTORY_H
#define HDNUKE_KNOBFACTORY_H

#include <unordered_map>

#include <pxr/pxr.h>

#include <pxr/base/vt/value.h>

#include <DDImage/Knobs.h>


using namespace DD::Image;


PXR_NAMESPACE_OPEN_SCOPE


class HdNukeKnobFactory
{
public:
    Knob* VtValueKnob(Knob_Callback f, const std::string& name,
                      const std::string& label, const VtValue& value);

    void FreeDynamicKnobStorage();

    static VtValue KnobToVtValue(Knob* knob);

private:
    std::unordered_map<std::string, std::unique_ptr<int>> _intKnobStorage;
    std::unordered_map<std::string, std::unique_ptr<float[]>> _floatKnobStorage;
    std::unordered_map<std::string, std::unique_ptr<double[]>> _doubleKnobStorage;
    std::unordered_map<std::string, std::unique_ptr<bool>> _boolKnobStorage;
    std::unordered_map<std::string, std::unique_ptr<std::string>> _stringKnobStorage;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_KNOBFACTORY_H
