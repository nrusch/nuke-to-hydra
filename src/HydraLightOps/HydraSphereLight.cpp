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
#include <GL/glew.h>

#include <pxr/pxr.h>

#include <pxr/imaging/hd/tokens.h>

#include <DDImage/Knobs.h>

#include <hdNuke/lightOp.h>


using namespace DD::Image;
PXR_NAMESPACE_USING_DIRECTIVE


static const char* const CLASS = "HydraSphereLight";
static const char* const HELP = "A spherical light.";


class HydraSphereLight : public HydraLightOp
{
public:
    HydraSphereLight(Node* node);
    ~HydraSphereLight() override { }

    void makeLightKnobs(Knob_Callback f) override;

    const TfToken& GetPrimTypeName() const override {
        return HdPrimTypeTokens->sphereLight;
    }

    const char* Class() const override { return CLASS; }
    const char* node_help() const override { return HELP; }

    static const Op::Description desc;

private:
    float _radius = 0.5f;
    bool _treatAsPoint = false;
};


static Op* build(Node* node) { return new HydraSphereLight(node); }
const Op::Description HydraSphereLight::desc(CLASS, 0, build);


HydraSphereLight::HydraSphereLight(Node* node)
    : HydraLightOp(node)
{
}

void
HydraSphereLight::makeLightKnobs(Knob_Callback f)
{
    HydraLightOp::makeLightKnobs(f);

    Float_knob(f, &_radius, "radius");
    SetRange(f, 0.05, 2);

    Bool_knob(f, &_treatAsPoint, "treatAsPoint", "treat as point");
    SetFlags(f, Knob::STARTLINE);
}
