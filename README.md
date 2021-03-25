# Nuke to Hydra

This project implements a Hydra scene delegate for Nuke's 3D system, as well as
ops to facilitate rendering 3D scenes using available Hydra render delegates.

## A Note About Nuke 13

The scene delegate implementation in this project was adopted by Foundry to
underpin the [Hydra Viewer in Nuke 13](
https://learn.foundry.com/nuke/content/release_notes/nuke_13.0.html), which is
a big part of why this repo has been quiet for so long. I'm not sure where
things will go from here, but I hope to be able to collaborate with them more
in the future, in order to port and implement other features and goals from this
project into Nuke, such as switchable render delegate support, the renderer op,
USD stage side-loading, SDR support, etc.

Ultimately, the goal of this project was always to try and move the needle on
getting a more modern 3D system into Nuke, and I see this as a promising first
step. :)

**WARNING**: This project is in a fairly volatile state. I'm not totally
satisfied with various aspects of the design, and there are a lot of things that
still need to be implemented, so please don't rely on anything remaining the way
it is. In other words, I reserve the right to force-push to master.

If all that hasn't scared you away, and you have suggestions or are interested
in contributing, feel free to open an issue or submit a PR. I ask that PRs be at
least somewhat substantive (no pure formatting/whitespace/comment/spelling
updates), and I reserve the right to not accept them.

## (Mostly) Supported Features

- Meshes
- Particles
- Mesh instancing via particle systems (mostly)
- Lights
    - A set of custom Hydra light ops is included, but Nuke's native lights also
    generally work (though some knobs map awkwardly to the light schema params).
- Render delegate settings (mostly)

## Unsupported/TODO

- Material authoring (https://github.com/nrusch/nuke-to-hydra/issues/6)
- Texturing (https://github.com/nrusch/nuke-to-hydra/issues/1)
- Motion blur (https://github.com/nrusch/nuke-to-hydra/issues/2)
- HdStorm support

## Building

I have developed and tested the project entirely on Centos 7 so far. I plan to
attempt a Windows build at some point, but trying to find old versions of the
MSVC compiler to download (2015 Update 3 in Nuke's case) has proved to be quite
a pain.

At any rate, if you want to build and test the project, you'll need the
following:

- GCC 4.8.5+
- CMake 3.0+
- Nuke 11.3+
- USD 20.02+
    - I'm not trying to support multiple USD versions at this point, and I want
    to stay reasonably current as Hydra continues to mature, so I'll
    occasionally jump ahead to a newer cut of the USD dev branch (see above note
    about project volatility).
- Some of the USD dependencies (for headers)
    - Boost Python (including headers)
    - Python
    - TBB

After that, building should be pretty simple:

```
cmake \
-D CMAKE_INSTALL_PREFIX=<YOUR_INSTALL_PREFIX> \
-D Nuke_ROOT=<YOUR_NUKE_INSTALL_ROOT> \
-D PXR_USD_LOCATION=<YOUR_USD_INSTALL_ROOT> \
<SOURCE_DIR>

make -j 4 install
```

If you need to specify where some of the USD dependencies are located, or if
they are not being found properly, try setting some of these CMake variables as
needed:

- `TBB_ROOT_DIR`
- `BOOST_ROOT`
- `CMAKE_PREFIX_PATH`

## Testing

To test the render op, you'll need at least one render delegate that *isn't
HdStorm* (I've been using Arnold and Embree).

Make sure the path to the delegate's plugin library is added to
`PXR_PLUGINPATH_NAME`, and add the installed `plugins` directory to `NUKE_PATH`.
Then launch Nuke, create a HydraRender node, and cross your fingers.
