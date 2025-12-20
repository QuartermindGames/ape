<div align="center">

![Logo](resources/ape_logo_v4_256.png)

</div>

ApeTech is a 3D game engine written in C23, being developed by 
[Mark "hogsy" Sowden](https://hogsy.me/), for game jams and prototyping.

ApeTech is an acronym for *"Another Portal Engine"*; meaning it uses a [portal-based renderer](https://en.wikipedia.org/wiki/Portal_rendering) in the same vein as some other engines, such as Red Faction, in which areas of the world are split into sectors (or rooms) which are then joined together by a portal plane, which is used to determine visibility.

What makes ApeTech a little different in this regard is that it's attempting to go a step further with this, aiming to replicate something similar to what 3D Realms' Prey from 1998 was attempting to do; joining rooms don't necessarily have to be connected physically but can be travelled between via portals that can be added and moved around dynamically.

It's available here with absolutely no support whatsoever. Additionally, it is not intended as anything close to a professional grade engine but instead just 
something that's easy to throw things at and modify. 
It's being developed primarily for **fun**.

Some semblance of documentation can be found [here](docs).

* [Features](#features)
* [Games](#games)
* [Screenshots](#screenshots)
* [Building](#building)
* [Q&A](#qa)

----

## Features

- Integration with our [qmfw](https://github.com/QuartermindGames/hei) library
  - PNG, TGA, JPG, BMP and GIF image support
  - Abstract graphics interface with support for different graphics APIs
  - GLSL pre-processor with support for directives such as `include`
  - Virtual file-system allowing for directories and packages to be mounted at runtime
- Console interface, with auto-completion, commands and variables
- Flexible material system providing support for outlining multiple passes, blend modes and more
- Custom package format with compression
- Custom image format called `GFX` with own "block" compression and support for DXTC
- Simple post-processing pipeline with support for FXAA, bloom, depth-of-field and more
- Super-sampling up to 2x display resolution
- Memory manager with garbage collection and usage tracking
- [ACM (Another Config Markup)](https://github.com/QuartermindGames/acm) for serialisation/deserialisation; can be stored as either binary or text
- Editor frontend, dubbed _Forge_, using [FOX Toolkit](http://www.fox-toolkit.org/)

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

Keep in mind that the code is taking advantage of C23 additions. 
To my knowledge, this currently means you're going to be limited to GCC 13 minimum, at least as of April 2024. 
You might have some success with Clang. 

I've done this because I'd estimate by the time this code is actually useful to anyone, if ever, C23 support should hopefully be reasonably widespread. 
If you've received this before that time, apologies!

The project uses CMake, so ideally it should be as simple as this...

```
mkdir build
cd build
cmake ../
```

Besides the platforms listed below, anything else is currently unsupported.
Historically, the engine had been successfully built and run on macOS—but that was quite a few years ago. Given Apple's recent actions and lack of support for open standards, such as Vulkan/OpenGL, I've felt less inclined to support it as a target.

### Linux

This is the primary target platform.

The engine has been tested against 64-bit [Ubuntu 24.04 LTS](https://ubuntu.com/download/desktop), and while it has not been tested against other distributions, is expected to work just fine.

### Windows

The engine primarily supports 64-bit Windows 11; it has not been tested against other versions of Windows, so your milage might vary if that's the case.
Additionally, much of the development is done on Linux, so Windows support often lags behind a bit.

In an ideal scenario, compilation requires just [MSYS2](https://www.msys2.org/) and [MinGW64](https://packages.msys2.org/groups/mingw-w64-x86_64-toolchain).

Alternatively, if you insist on using MSVC, there's a `setup_project_msvc.bat` available.
This requires CMake, Visual Studio and Clang (installed via Visual Studio).

## Q&A

### Why is the commit history not available?

Depending on how you got this, I've likely not made the commit history available because there is a lot of experimental work I'll typically do, and I'm often not terribly happy with it.
So essentially, what you're seeing is the "clean" version.

### Can I contribute to the project?

I'm afraid I'm not willing to accept contributions.
I appreciate the notion though!
But for the same reason as above, this isn't the _current_ version of the engine - as a matter of fact, I'll usually make any copies public a year after a milestone is hit, so what you're seeing is already pretty dated.

### What makes this better than engine _X_?

Nothing!
It's not trying to be better than anything.
I've got no expectation it ever will be.

This was developed to suit and fit my needs, nothing more and nothing less.

### Why did you write it in C?

I'd decided to write a 3D engine in C just for the novelty of it, and because I've got a soft spot for the simplicity of the language.

That said, in hindsight, this would've been written in C++ had I started this project today.
Maybe for a future version I'll try migrating to C++, though at this stage it'd be a lot of work.
