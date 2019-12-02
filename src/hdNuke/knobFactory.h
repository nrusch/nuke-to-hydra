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

    // VtValue KnobToVtValue(const std::string& name) const;

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
