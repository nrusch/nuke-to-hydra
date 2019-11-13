# HydraNuke

Hydra rendering in Nuke.

## TODO

#### HydraRender Op

- Higher bit depth output
- Add knobs to control output of depth, normals.
    - Support both depth and linearDepth (need to manually add the latter)
- Add a knob to output all buffers for debugging.
- Dynamic knobs for render delegate settings
- Add knob to enable/disable lights, materials
- Handle aborted() in engine during render
- Manual abort mechanism for renders (cancel button?)
    - Keep an eye out for any way to hook a progress callback to Hydra
- Produce some metadata
- Storm support (2D and/or 3D)

#### Scene Delegate

- GetExtent impl
- Particles
    - Need a fast way to identify GeoInfo type without having to look at all
      child primitives.
    - Need a render delegate that supports points in order to test this
- Do we need to worry about `AttribContext.varying`?
- Materials
    - UsdPreviewSurface
    - Basic rprim colors from Nuke
    - Texturing support.
        - Render Nuke's material Iops to texture buffers (or temp files)
        - ExtComputations may be part of it...
- Time handling (for motion blur, etc.)

#### Misc Notes

- Might need/want to implement custom Hydra light ops
- Op to draw USD stage into the 3D viewer without creating geo
