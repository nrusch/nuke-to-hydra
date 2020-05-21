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
#include <random>

#include <GL/glew.h>

#include <pxr/pxr.h>

#include <pxr/base/gf/rotation.h>

#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <DDImage/AxisOp.h>
#include <DDImage/Knobs.h>

#include <hdNuke/hydraOpManager.h>
#include <hdNuke/opBases.h>
#include <hdNuke/utils.h>


using namespace DD::Image;
PXR_NAMESPACE_USING_DIRECTIVE


static const char* const CLASS = "HydraUSDStage";
static const char* const HELP = "Loads the contents of a USD stage directly "
                                "into a Hydra render index.";


class HydraUSDStage : public AxisOp, public HydraOp
{
public:
    HydraUSDStage(Node* node);
    ~HydraUSDStage() override { }

    void knobs(Knob_Callback f) override;
    int knob_changed(DD::Image::Knob* k) override;

    const char* Class() const override { return CLASS; }
    const char* node_help() const override { return HELP; }

    void Populate(HydraOpManager* manager) override;

    static const Op::Description desc;

private:
    const char* _stagePath;
    std::string _rootPrimPath;

    UsdStageRefPtr _stage;
    SdfPath _delegateId;

    bool _needStageReload = true;
    bool _needDelegateRebuild = true;

    static std::default_random_engine RNG;
};


static Op* build(Node* node) { return new HydraUSDStage(node); }
const Op::Description HydraUSDStage::desc(CLASS, 0, build);
std::default_random_engine HydraUSDStage::RNG;


HydraUSDStage::HydraUSDStage(Node* node)
    : DD::Image::AxisOp(node)
    , HydraOp()
    , _stagePath("")
    , _rootPrimPath("/")
{
    Hash idHash;
    idHash.append(static_cast<uint32_t>(RNG()));
    std::ostringstream buf;
    buf << "/USDStage_" << std::hex << idHash.value();
    _delegateId = SdfPath(buf.str());
}

void
HydraUSDStage::knobs(Knob_Callback f)
{
    File_knob(f, &_stagePath, "stage_path", "stage path");
    String_knob(f, &_rootPrimPath, "root_prim_path", "root prim path");

    // TODO: Time controls
    // TODO: Population masking

    Divider(f);
    AxisOp::knobs(f);
}

int
HydraUSDStage::knob_changed(Knob* k)
{
    if (k->is("stage_path")) {
        _needStageReload = true;
        _needDelegateRebuild = true;
        return 1;
    }

    if (k->is("root_prim_path")) {
        if (_rootPrimPath.empty()) {
            k->set_text("/");
        }

        _needDelegateRebuild = true;
        return 1;
    }

    return AxisOp::knob_changed(k);
}


void
HydraUSDStage::Populate(HydraOpManager* manager)
{
    const bool emptyStagePath = strlen(_stagePath) == 0;

    if (_needDelegateRebuild) {
        manager->RemoveUsdDelegate(_delegateId);
        _needDelegateRebuild = false;

        if (_needStageReload) {
            if (not emptyStagePath) {
                _stage = UsdStage::Open(_stagePath);
            }
            else {
                _stage.Reset();
            }
            _needStageReload = false;
        }
    }

    if (emptyStagePath) {
        error("Empty stage path");
        return;
    }

    if (not _stage) {
        error("Failed to open stage path %s", _stagePath);
        return;
    }

    bool repopulateDelegate = false;
    UsdImagingDelegate* delegate = manager->GetUsdDelegate(_delegateId);
    if (not delegate) {
        delegate = manager->CreateUsdDelegate(_delegateId);
        repopulateDelegate = true;
    }

    GfMatrix4d rootXform(1);
    // Basic up-axis correction for Z-up stages
    if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
        rootXform.SetRotate(GfRotation({1, 0, 0}, -90));
    }

    rootXform *= DDToGfMatrix4d(matrix());
    delegate->SetRootTransform(rootXform);

    delegate->SetTime(outputContext().frame());

    if (repopulateDelegate) {
        SdfPath rootPrimPath(_rootPrimPath);
        if (not (rootPrimPath.IsAbsolutePath()
                 and rootPrimPath.IsAbsoluteRootOrPrimPath())) {
            error("Not a valid absolute root or prim path: %s",
                  _rootPrimPath.c_str());
            return;
        }

        UsdPrim rootPrim = _stage->GetPrimAtPath(rootPrimPath);
        if (not rootPrim) {
            error("Could not find prim at path: %s", _rootPrimPath.c_str());
            return;
        }

        delegate->Populate(rootPrim);
    }
}
