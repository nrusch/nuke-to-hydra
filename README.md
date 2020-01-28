# Nuke to Hydra

This project implements a Hydra scene delegate for Nuke's 3D system, as well as
ops to facilitate rendering 3D scenes using available Hydra render delegates.

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
    - Light support currently piggybacks on Nuke's native light ops, but I plan
    to implement dedicated Hydra light ops in the future (see
    https://github.com/nrusch/nuke-to-hydra/issues/3).
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

- GCC 4.8.5
- CMake 3.0+
- Nuke
    - I started developing this against 11.2, but I imagine earlier versions
    would work as well, since the 3D API hasn't really changed.
- USD 20.02+
    - I'm not trying to support multiple USD versions at this point, and I want
    to stay reasonably current as Hydra continues to mature, so I'll
    occasionally jump ahead to a newer cut of the USD dev branch (see above note
    about project volatility).

After that, building is pretty simple:

```
cmake \
-D CMAKE_INSTALL_PREFIX=<YOUR_INSTALL_PREFIX> \
-D Nuke_ROOT=<YOUR_NUKE_INSTALL_ROOT> \
-D USD_ROOT=<YOUR_USD_INSTALL_ROOT> \
<SOURCE_DIR>

make -j 4 install
```

## Testing

To test the render op, you'll need at least one render delegate that *isn't
HdStorm* (I've been using Arnold and Embree).

Make sure the path to the delegate's plugin library is added to
`PXR_PLUGINPATH_NAME`, and add the installed `plugins` directory to `NUKE_PATH`.
Then launch Nuke, create a HydraRender node, and cross your fingers.
