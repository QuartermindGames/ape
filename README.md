<div align="center">

![Logo](resources/ape_logo_v4_256.png)

</div>

APE is a 3D game engine written in C23, being developed by 
[Mark "hogsy" Sowden](https://hogsy.me/), for game jams and prototyping.

APE is an acronym for *"Another Portal Engine"*; meaning it uses a [portal-based renderer](https://en.wikipedia.org/wiki/Portal_rendering) in the same vein as some other engines, such as Red Faction, in which areas of the world are split into sectors (or rooms) which are then joined together by a portal plane, which is used to determine visibility.

What makes APE a little different in this regard is that it's attempting to go a step further with this, aiming to replicate something similar to what 3D Realms' Prey from 1998 was attempting to do; joining rooms don't necessarily have to be connected physically but can be travelled between via portals that can be added dynamically.

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
- Node format for serialisation/deserialisation; can be stored as either binary or text
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

## Building

I've been primarily using GCC as a compiler and have taken advantage of a few extensions available there that might not be available elsewhere. Clang should be alright. MSVC might have issues.

The project uses CMake, so ideally it should be as simple as this...
```
mkdir build
cd build
cmake ../
```

### Windows

APE Tech primarily supports 64-bit Windows 11; it has not been tested against other versions of Windows, so your milage might vary if that's the case.

Compilation requires [MSYS2](https://www.msys2.org/) and [MinGW64](https://packages.msys2.org/groups/mingw-w64-x86_64-toolchain). As mentioned, Clang should also work, but I've not tried compiling it with Clang myself.

### Linux

The engine primarily supports 64-bit Ubuntu 23.10 and has not been tested against other distributions of Linux, but is expected to work just fine.

### macOS

Historically, the engine had been successfully built and run on macOS—but that was quite a few years ago. Given Apple's recent actions and lack of support for open standards, such as Vulkan/OpenGL, I've felt less inclined to support it as a target.

## Roadmap

