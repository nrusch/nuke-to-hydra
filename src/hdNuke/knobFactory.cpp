#include "knobFactory.h"

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>


PXR_NAMESPACE_OPEN_SCOPE


Knob*
HdNukeKnobFactory::VtValueKnob(Knob_Callback f, const std::string& name,
                               const std::string& label, const VtValue& value)
{
    if (value.IsEmpty()) {
        return nullptr;
    }

    if (value.IsArrayValued()) {
        TF_WARN("Array value types are not supported for knob conversion: %s",
                value.GetTypeName().c_str());
        return nullptr;
    }

    // TODO: Vector types
    size_t numElements = 1;

    Knob* result = nullptr;

    // TODO: Template this
    if (value.IsHolding<int>()) {
        int* data;
        auto it = _intKnobStorage.find(name);
        if (it == _intKnobStorage.end()) {
            std::unique_ptr<int> storage(new int(value.UncheckedGet<int>()));
            data = storage.get();
            _intKnobStorage.emplace(name, std::move(storage));
        }
        else {
            data = it->second.get();
        }

        Int_knob(f, data, name.c_str(), label.c_str());
    }
    else if (value.IsHolding<float>()) {
        float* data;
        auto it = _floatKnobStorage.find(name);
        if (it == _floatKnobStorage.end()) {
            std::unique_ptr<float[]> storage(new float[numElements]);
            data = storage.get();
            data[0] = value.UncheckedGet<float>();
            _floatKnobStorage.emplace(name, std::move(storage));
        }
        else {
            data = it->second.get();
        }

        Float_knob(f, data, name.c_str(), label.c_str());
    }
    else if (value.IsHolding<double>()) {
        double* data;
        auto it = _doubleKnobStorage.find(name);
        if (it == _doubleKnobStorage.end()) {
            std::unique_ptr<double[]> storage(new double[numElements]);
            data = storage.get();
            data[0] = value.UncheckedGet<double>();
            _doubleKnobStorage.emplace(name, std::move(storage));
        }
        else {
            data = it->second.get();
        }

        Double_knob(f, data, name.c_str(), label.c_str());
    }
    else if (value.IsHolding<bool>()) {
        bool* data;
        auto it = _boolKnobStorage.find(name);
        if (it == _boolKnobStorage.end()) {
            std::unique_ptr<bool> storage(new bool(value.UncheckedGet<bool>()));
            data = storage.get();
            _boolKnobStorage.emplace(name, std::move(storage));
        }
        else {
            data = it->second.get();
        }

        Bool_knob(f, data, name.c_str(), label.c_str());
    }
    else if (value.IsHolding<std::string>()) {
        std::string* data;
        auto it = _stringKnobStorage.find(name);
        if (it == _stringKnobStorage.end()) {
            std::unique_ptr<std::string> storage(
                new std::string(value.UncheckedGet<std::string>()));
            data = storage.get();
            _stringKnobStorage.emplace(name, std::move(storage));
        }
        else {
            data = it->second.get();
        }

        String_knob(f, data, name.c_str(), label.c_str());
    }
    else {
        TF_WARN("Unsupported value type for knob conversion: %s",
                value.GetTypeName().c_str());
    }
    return result;
}

void
HdNukeKnobFactory::FreeDynamicKnobStorage()
{
    _intKnobStorage.clear();
    _floatKnobStorage.clear();
    _doubleKnobStorage.clear();
    _boolKnobStorage.clear();
    _stringKnobStorage.clear();
}

/* static */
VtValue
HdNukeKnobFactory::KnobToVtValue(Knob* knob)
{
    if (not knob) {
        return VtValue();
    }

    switch(knob->ClassID()) {
        case FLOAT_KNOB:
            return VtValue(static_cast<float>(knob->get_value()));
        case DOUBLE_KNOB:
            return VtValue(knob->get_value());
        case BOOL_KNOB:
            return VtValue(static_cast<bool>(knob->get_value()));
        case INT_KNOB:
        case ENUMERATION_KNOB:
            return VtValue(static_cast<int>(knob->get_value()));
        case COLOR_KNOB:
        case XYZ_KNOB:
            return VtValue(GfVec3f(knob->get_value(0), knob->get_value(1),
                                   knob->get_value(2)));
        case ACOLOR_KNOB:
            return VtValue(GfVec4f(knob->get_value(0), knob->get_value(1),
                                   knob->get_value(2), knob->get_value(3)));
        case STRING_KNOB:
        case FILE_KNOB:
            return VtValue(knob->get_text());
        default:
            TF_WARN("KnobToVtValue : No VtValue conversion implemented for "
                    "knob type ID: %d", knob->ClassID());
            return VtValue();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
