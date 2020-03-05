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
#include <pxr/pxr.h>

#include <DDImage/Op.h>

#include <hdNuke/opBases.h>


using namespace DD::Image;
PXR_NAMESPACE_USING_DIRECTIVE


static const char* const CLASS = "HydraScene";
static const char* const HELP = "Merges multiple Hydra ops.";


class HydraScene : public Op, public HydraOp
{
public:
    HydraScene(Node* node);
    ~HydraScene() override { }

    int minimum_inputs() const override { return 1; }
    int maximum_inputs() const override { return 99; }
    int optional_input() const override { return 1; }
    bool test_input(int index, Op* op) const override;

    void append(Hash& hash) override;

    void _validate(bool for_real) override;

    const char* node_shape() const override { return "O"; }

    const char* Class() const override { return CLASS; }
    const char* node_help() const override { return HELP; }

    void Populate(HydraOpManager* manager) override;

    static const Op::Description desc;
};


static Op* build(Node* node) { return new HydraScene(node); }
const Op::Description HydraScene::desc(CLASS, 0, build);


HydraScene::HydraScene(Node* node)
    : Op(node)
    , HydraOp()
{
}

bool
HydraScene::test_input(int index, Op* op) const
{
    return dynamic_cast<HydraOp*>(op) != nullptr;
}

void
HydraScene::append(Hash& hash)
{
    for (Op* inputOp : getInputs())
    {
        if (inputOp) {
            inputOp->append(hash);
        }
    }
}

void
HydraScene::_validate(bool for_real)
{
    for (Op* inputOp : getInputs())
    {
        if (inputOp) {
            inputOp->validate(for_real);
        }
    }
}

void
HydraScene::Populate(HydraOpManager* manager)
{
    for (Op* inputOp : getInputs())
    {
        if (inputOp and inputOp->valid()) {
            // XXX: Can we avoid a dynamic_cast here?
            dynamic_cast<HydraOp*>(inputOp)->Populate(manager);
        }
    }
}
