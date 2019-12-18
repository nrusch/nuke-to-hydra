# HydraNuke

Hydra rendering in Nuke.

## TODO

#### HydraRender Op

- Work on getting more reliable cached output
- Add knobs to control output of depth, normals.
    - Support both depth and linearDepth (need to manually add the latter)
- Add a knob to output all buffers for debugging.
- "Manual Update" mode
- Add knobs to enable/disable lights, materials
- Produce some metadata
- Handle aborted() in engine during render
- Manual abort mechanism for renders (cancel button?)
    - Keep an eye out for any way to hook a progress callback to Hydra
- Storm support (2D and/or 3D)
- Add some diagnostic display modes
- Option to use default preview surface for display color?

#### Scene Delegate

- Particles
    - Need to support instancing (for mesh scattering particle systems)
- SampleTransform (motion blur) / SamplePrimvar
    - Need a render delegate that actually supports multiple samples first...
- Materials
    - UsdPreviewSurface
    - Basic rprim colors from Nuke
    - Texturing support.
        - Render Nuke's material Iops to texture buffers (or temp files)
        - ExtComputations may be part of it...
- Background color/image?

#### Other

- Might want to implement custom Hydra light ops
- Op to load USD stage directly into render index, draw USD stage into 3D
  viewer
    - Create a generic base class for "scene delegate" ops?
