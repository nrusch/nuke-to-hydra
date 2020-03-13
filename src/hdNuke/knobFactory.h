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


PXR_NAMESPACE_OPEN_SCOPE


template <typename T>
class KnobDataStore
{
public:

    typedef std::unique_ptr<T> UniquePtrType;

    T* FindOrAllocate(const std::string& name, const VtValue& initValue)
    {
        T* result = nullptr;
        auto it = _data.find(name);
        if (it == _data.end()) {
            UniquePtrType storage(new T(initValue.UncheckedGet<T>()));
            result = storage.get();
            _data.emplace(name, std::move(storage));
        }
        else {
            result = it->second.get();
        }
        return result;
    }

    inline void Clear() { _data.clear(); }

private:
    std::unordered_map<std::string, UniquePtrType> _data;
};


class HdNukeKnobFactory
{
public:
    DD::Image::Knob* VtValueKnob(DD::Image::Knob_Callback f,
                                 const std::string& name,
                                 const std::string& label,
                                 const VtValue& value);

    void FreeDynamicKnobStorage();

private:
    KnobDataStore<int> _intKnobStorage;
    KnobDataStore<float> _floatKnobStorage;
    KnobDataStore<double> _doubleKnobStorage;
    KnobDataStore<bool> _boolKnobStorage;
    KnobDataStore<std::string> _stringKnobStorage;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_KNOBFACTORY_H
