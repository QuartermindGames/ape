# Todo

## Features
- ~~**forge:** Add a filter menu to the material browser~~
- **core:** Introduce flags for shaders to indicate support for lighting
- ~~**core:** Expose node properties to the editor~~
  - **core:** Allow us to modify and save node properties (possibly need to revise API)
- Add a way to move the grid around
- Vertex editing
- Extrude for brushes
- Move for brushes
- Implement a new tab type with close button, so we can easily close editors
- Editor is crashing when attempting to close an editor tab (looks like an issue with queued job?)
- Make the console correctly hide/show, and resize with a window

## Bugs
- **forge**: Crash when deleting some objects such as lights (probably due to queue)

## Per-pixel audio maps
A 16x16 texture (or tiny relative to parent texture) that outlines on a per-pixel basis what sound should be used.
These can be loaded into RAM and then when an object is on a surface, query the relative UV x and y to the object and determine pixel colour in order to get the correct sound.
This means that if you have a texture that transitions or has both a wood and metal surface, the correct sound can play relative to where on that texture you're standing.
System can also be expanded to support height-maps once introduced.

## Queue
- Initial Lua mock-up
- ~~Restrict vertices for stencil shadows based on point of intersection~~
  - ~~If point doesn't cross/intersect, we should have a reasonable limit~~
- ~~Mirrors—correctly traverse from room to build a visible list of rooms w/ transforms~~
  - Recursion for portals/mirrors isn't working right
  - ~~Can't combine stencil shadow volumes with mirrors~~ (for now we'll just disable shadows in mirrors)
- ~~We don't have spotlights...~~
- Prototype 'eyes' for QM2 & QM1
  - Plane with two layers, the second layer representing pupil will take offset for eye movement. Maybe the first layer acts as a mask?
  - Add some logic for swapping out first texture, so we can convey different expressions
- Implement DetachShaderStage in graphics driver
- Allow for reloading materials on command
- Skybox w/ clouds
  - Sun/Moon or general sprites can be mapped to points in the skybox?
- String table for binary-based node format
- Finish implementing support for the receive shadow flag (might need self-shadow too)
- Handler for team colours via shader; some sort of mask
- Pass vertex weights as attributes for vertex shader
- Make attributes automatically derive from shader; open up API for specifying additional attributes (see vertex-desc branch per Hei)
