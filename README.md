<div align="center">

![Logo](resources/ape_logo_v4_256.png)

</div>

APE is a simplistic 3D game engine, being developed by 
[Mark "hogsy" Sowden](https://hogsy.me/), 
for game jams and prototyping.
APE is an acronym for *"Another Portal Engine"*; meaning it uses a [portal-based renderer](https://en.wikipedia.org/wiki/Portal_rendering) in the same vein as some other engines, such as Red Faction, in which areas of the world are split into sectors (or rooms) which are then joined together by a portal plane, which is used to determine visibility.

It's available here with absolutely no support whatsoever. Additionally, it is not intended as anything close to a professional grade engine but instead just 
something that's easy to throw things at and modify. 
It's being developed primarily for **fun**.

Some semblance of documentation can be found [here](docs).

## Features

- Integration with [Hei Platform Library](https://github.com/OldTimes-Software/hei)
  - Plugins for supporting additional package and texture formats
  - PNG, TGA, JPG, BMP and GIF image support
  - Abstract graphics interface with support for different graphics APIs via plugins
  - GLSL pre-processor with support for directives such as `include`
  - Virtual file-system allowing for directories and packages to be mounted at runtime
- Console interface, with auto-completion, commands and variables
- Flexible material system providing support for outlining multiple passes, blend modes and more
- Custom package format with compression
- Custom image format called `GFX` with own "block" compression and support for DXTC
- Simple post-processing pipeline with support for FXAA and bloom
- Super-sampling up to 2x display resolution
- Memory manager with garbage collection and usage tracking
- Node format for serialization/deserialization; can be stored as either binary or text
- Editor frontend using [FOX Toolkit](http://www.fox-toolkit.org/)
- Compatibility with Red Faction engine
  - VPP v1, v2 and v3

## Games

Below is a list of released games that have used this engine.

- [Space Ranger: Asteroid Attack](https://hogsy.itch.io/space-ranger-asteroid-attack)
- [Buddy's Adventure](https://hogsy.itch.io/buddy)

## Screenshots

<div align="center">

[![Screenshot](resources/preview0_thumb.png)](resources/preview0.png)
[![Screenshot](resources/sr_preview_thumb.png)](resources/sr_preview.png)
[![Screenshot](resources/preview2_thumb.png)](resources/preview2.png)
[![Screenshot](resources/preview3_thumb.png)](resources/preview3.png)

</div>

## Building

### Windows

APE Tech primarily supports 64-bit Windows 11, and has not been tested against other versions of Windows, so your milage might vary if that's the case.

Compilation of tools requires [MSYS2](https://www.msys2.org/), [MinGW64](https://packages.msys2.org/groups/mingw-w64-x86_64-toolchain) and [GTK4](https://packages.msys2.org/package/mingw-w64-x86_64-gtk4?repo=mingw64). All other dependencies are included.

### Linux

APE Tech primarily supports 64-bit Ubuntu 23.04 and has not been tested against other distributions of Linux, but is expected to work just fine.

Compilation of tools requires GTK4.

## Roadmap

### Red Faction Support
One of the extra goals of the engine is support for elements Volition developed for their own in-house engine.

#### Formats
- VBM (**pending**)
- VF (**pending**)
- VFX (**pending**)
- VPP v1 (**done!**) (The Summoner, Red Faction)
- VPP v2 (**done!**) (Red Faction II)
- VPP v2+ (**pending**) (The Summoner 2)
- VPP v3 (**done!**) (The Punisher)
- RFA (**pending**)
- RFMC v1 (**in-progress**) (Red Faction)
- RFMC v10 (**in-progress**) (Red Faction II)
- PEG (**pending**) (Red Faction, Red Faction II, The Summoner 2, The Punisher)

#### Features
- Realtime environmental CSG (**pending**) (Red Faction, Red Faction II)
- Lightmaps (**pending**) (The Summoner, Red Faction)
