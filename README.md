# APE (Another Portal Engine)

<div align="center">

![Logo](resources/icon.png)

</div>

APE is a simplistic 3D game engine, being developed by 
[Mark "hogsy" Sowden](https://hogsy.me/), 
for game jams and prototyping.
APE is an acronym for *"Another Portal Engine"*; meaning it uses a [portal-based renderer](https://en.wikipedia.org/wiki/Portal_rendering) in the same vein as other engines such as Red Faction, in which areas of the world are split into sectors (or rooms) which are then joined together by a portal plane, which is used to determine visibility.

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
