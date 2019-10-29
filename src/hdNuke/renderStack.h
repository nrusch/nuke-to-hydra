#ifndef HDNUKE_RENDERSTACK_H
#define HDNUKE_RENDERSTACK_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdx/taskController.h>

#include <pxr/usdImaging/usdImaging/delegate.h>

#include "sceneDelegate.h"


PXR_NAMESPACE_OPEN_SCOPE


struct HydraRenderStack
{
    HdRendererPlugin* rendererPlugin = nullptr;  // Ref-counted by Hydra
    HdRenderIndex* renderIndex = nullptr;

    HdNukeSceneDelegate* nukeDelegate = nullptr;
    UsdImagingDelegate* usdDelegate = nullptr;

    HdxTaskController* taskController = nullptr;
    HdRprimCollection primCollection;

    HydraRenderStack(HdRendererPlugin* pluginPtr);

    ~HydraRenderStack();

    static HydraRenderStack* Create(TfToken pluginId);

    // XXX: For testing
    void loadUSDStage(const char* usdFilePath);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_RENDERSTACK_H
