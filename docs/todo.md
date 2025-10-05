# Todo

## Features
- Extrude for brushes

## Per-pixel audio maps
A 16x16 texture (or tiny relative to parent texture) that outlines on a per-pixel basis what sound should be used.
These can be loaded into RAM and then when an object is on a surface, query the relative UV x and y to the object and determine pixel colour in order to get the correct sound.
This means that if you have a texture that transitions or has both a wood and metal surface, the correct sound can play relative to where on that texture you're standing.
System can also be expanded to support height-maps once introduced.

## Queue
- **core**: emissive materials
- **forge/core**: autosave current world on error
- **forge**: implement a new tab type with close button, so we can easily close editors
- **forge**: make the console correctly hide/show, and resize with a window
- Initial Lua mock-up
- ~~Mirrors—correctly traverse from room to build a visible list of rooms w/ transforms~~
  - Recursion for portals/mirrors isn't working right
- Prototype 'eyes' for QM2 & QM1
  - Plane with two layers, the second layer representing pupil will take offset for eye movement. Maybe the first layer acts as a mask?
  - Add some logic for swapping out first texture, so we can convey different expressions
- Implement DetachShaderStage in graphics driver
- Skybox w/ clouds
  - Sun/Moon or general sprites can be mapped to points in the skybox?
- String table for binary-based node format
- Finish implementing support for the receive shadow flag (might need self-shadow too)
- Handler for team colours via shader; some sort of mask
- Pass vertex weights as attributes for vertex shader
