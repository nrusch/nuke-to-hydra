# HydraNuke

Hydra rendering in Nuke.

## TODO

#### HydraRender Op

- Add knobs to control output of depth, normals.
    - Support both depth and linearDepth (need to manually add the latter)
- Add a knob to output all buffers for debugging.
- Handle aborted() in engine during render
- Manual abort mechanism for renders (cancel button?)
    - Keep an eye out for any way to hook a progress callback to Hydra
- Compare HydraData struct with UsdImagingGLEngine render plugin swapping code
- Look up renderer plugin by ID instead of keeping an ID token array
    - Can we store a string instead of an index?
- Storm support (2D and/or 3D)
- Different rendering modes (wireframe, etc)?
- Dynamic knobs for render delegate settings

#### Scene Delegate

- GetExtent impl
- Particles
    - Need a fast way to identify GeoInfo type without having to look at all
      child primitives.
    - Need a render delegate that supports points in order to test this
- Lights
- Remap known attribute names to align with USD/Hd standards
    - uv -> st
    - N -> HdTokens->normals
    - Cf -> HdTokens->displayColor
- Do we need to worry about `AttribContext.varying`?
- Nuke material handling
    - Basic rprim colors from Nuke
    - Texturing support. Can we use Nuke's material evaluation to generate
      texture buffers, and then somehow hand those over?
        - ExtComputations may be the answer...
- Partial rprim updates.
    - Need to understand what the GeoInfo src and out hashes indicate
    - Ask Foundry about this.
    - May need to re-think rprim IDs
    - If the source hash is the geo minus xforms or something, can we do
      instancing?
- Time handling (for motion blur, etc.)

#### Misc Notes

- Need other (more capable) render delegates to test
- Op to draw USD stage into the 3D viewer without creating geo
