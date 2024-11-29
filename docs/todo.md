# Todo

## Immediate
- Add a filter menu to the material browser
- Introduce flags for shaders to indicate support for lighting

## Editor
- Add a way to move the grid around
- Vertex editing
- Extrude for brushes
- Move for brushes
- Implement a new tab type with close button, so we can easily close editors
- Editor is crashing when attempting to close an editor tab (looks like an issue with queued job?)
- Make the console correctly hide/show, and resize with a window

## Queue
- Initial Lua mock-up
- Restrict vertices for stencil shadows based on point of intersection
  - If point doesn't cross/intersect, we should have a reasonable limit
- Mirrors—correctly traverse from room to build a visible list of rooms w/ transforms
- We don't have spotlights...
- Prototype 'eyes' for SS2 & SS1
  - Plane with two layers, the second layer representing pupil will take offset for eye movement. Maybe the first layer acts as a mask?
  - Add some logic for swapping out first texture, so we can convey different expressions
- Implement DetachShaderStage in graphics driver
- Allow for reloading materials on command
- Skybox w/ clouds
- String table for binary-based node format
- Finish implementing support for the receive shadow flag (might need self-shadow too)
- Handler for team colours via shader; some sort of mask
- Pass vertex weights as attributes for vertex shader
- Make attributes automatically derive from shader; open up API for specifying additional attributes (see vertex-desc branch per Hei)
