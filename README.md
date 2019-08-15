# HydraNuke

Hydra rendering in Nuke.

## TODO

#### HydraRender Op

- Convert to PlanarIop (maybe switchable at compile time)
- Handle aborted() in engine during render
- Manual abort mechanism for renders (cancel button?)
    - Keep an eye out for any way to hook a progress callback to Hydra
- Support all render buffer formats (Int32 will be weird)
- Add table knob for render delegate settings (at least initially)
- Silo render stack in some kind of a struct to make for easy setup/teardown
    - See if we can keep any pieces around when switching (and if it's worth it)
    - Double-check UsdImagingGLEngine render plugin swapping code
- Look up renderer plugin by ID instead of keeping an ID token array
    - Can we store a string instead of an index?
- Storm support (2D and/or 3D)
- Different rendering modes (wireframe, etc)?

#### Scene Delegate

- GetExtent impl
- Particles
    - Need a fast way to identify GeoInfo type without having to look at all
      child primitives.
- Lights
- Primvar descriptors/sampling
    - Pre-cache information about available attributes to make this easier
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
