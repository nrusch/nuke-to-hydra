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
#include <pxr/usd/usdLux/tokens.h>

#include <DDImage/Knobs.h>

#include <hdNuke/lightOp.h>


using namespace DD::Image;
PXR_NAMESPACE_USING_DIRECTIVE


static const char* const CLASS = "HydraDomeLight";
static const char* const HELP = "A dome light with an optional texture map.";

static const char* const textureFormatNames[] = {
    "automatic",
    "latlong",
    "mirroredBall",
    "angular",
    "cubeMapVerticalCross",
    0
};


class HydraDomeLight : public HydraLightOp
{
public:
    HydraDomeLight(Node* node);
    ~HydraDomeLight() override { }

    const char* Class() const override { return CLASS; }
    const char* node_help() const override { return HELP; }

    static const Op::Description desc;

protected:
    void MakeLightKnobs(Knob_Callback f) override;

private:
    const char* _textureFile;
    int _textureFormat = 0;
};


static Op* build(Node* node) { return new HydraDomeLight(node); }
const Op::Description HydraDomeLight::desc(CLASS, 0, build);


HydraDomeLight::HydraDomeLight(Node* node)
    : HydraLightOp(node, HdPrimTypeTokens->domeLight)
    , _textureFile("")
{
}

void
HydraDomeLight::MakeLightKnobs(Knob_Callback f)
{
    HydraLightOp::MakeLightKnobs(f);

    File_knob(f, &_textureFile, "texture_file", "texture file");
    RegisterLightParamKnob(f, UsdLuxTokens->textureFile);

    Enumeration_knob(f, &_textureFormat, textureFormatNames, "texture_format",
                     "texture format");
    RegisterLightParamKnob(f, UsdLuxTokens->textureFormat);
}
