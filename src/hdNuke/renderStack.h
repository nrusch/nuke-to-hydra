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
#ifndef HDNUKE_RENDERSTACK_H
#define HDNUKE_RENDERSTACK_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdx/taskController.h>

#include <pxr/usdImaging/usdImaging/delegate.h>

#include "sceneDelegate.h"


PXR_NAMESPACE_OPEN_SCOPE


class HydraRenderStack
{
public:
    HdRendererPlugin* rendererPlugin = nullptr;  // Ref-counted by Hydra
    HdRenderIndex* renderIndex = nullptr;

    HdNukeSceneDelegate* nukeDelegate = nullptr;
    UsdImagingDelegate* usdDelegate = nullptr;

    HdxTaskController* taskController = nullptr;
    HdRprimCollection primCollection;

    HydraRenderStack(HdRendererPlugin* pluginPtr);

    ~HydraRenderStack();

    inline HdRenderDelegate* GetRenderDelegate() const {
        return renderIndex->GetRenderDelegate();
    }

    std::vector<HdRenderBuffer*> GetRenderBuffers() const;

    static HydraRenderStack* Create(TfToken pluginId);

    // XXX: For testing
    void loadUSDStage(const char* usdFilePath);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_RENDERSTACK_H
