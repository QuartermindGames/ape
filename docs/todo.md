# Todo

## Future
- Look into PSK/PSA formats for cook tool
- Initial Lua mock-up
- Investigate a solution for giving stencil shadows a softer appearance
  - One I've seen is to essentially draw the result of the stencil shadow volumes to another buffer, blur it and then apply it to the scene; if we include depth information with this, we could use it for controlling the samples per depth (kind of like a DOF effect)
  - Another is to use a selection of wedges that are shaped based on the penumbra...
- Look into adding support for DOF
- Stencil shadow volumes only need to extend within the sphere/radius of a local light source
- Look into the light-bleeding issue...
- Restrict vertices for stencil shadows based on point of intersection
  - If point doesn't cross/intersect, we should have a reasonable limit

## Current
- Mirrors—correctly traverse from room to build a visible list of rooms w/ transforms
- We don't have spotlights...
- Prototype 'eyes' for SS2 & SS1
  - Plane with two layers, the second layer representing pupil will take offset for eye movement. Maybe the first layer acts as a mask?
  - Add some logic for swapping out first texture, so we can convey different expressions
- Implement DetachShaderStage in graphics driver
- Allow for reloading materials on command
- World deserialiser should perform deserialisation on context of a node tree, rather than explicitly by type
- 'current' world selection in editor
- **(Editor)** Different viewport implementations, as some logic for viewport isn't necessary for others
- **(Editor)** Add texture selection frame
- Implement a new tab type with close button, so we can easily close editors
- **(Editor)** Editor is crashing when attempting to close an editor tab (looks like an issue with queued job?)
- **(Editor)** Look into the concept of supporting multiple grids
- **(Editor)** Make the console correctly hide/show, and resize with a window
- Recycle the parser from Dickens for ACM
- Skybox w/ clouds
- String table for binary-based node format
- Update cook tool to convert obj geom to brush rather than room geo
- Finish implementing support for the receive shadow flag (might need self-shadow too)
- Handler for team colours via shader; some sort of mask
- Pass vertex weights as attributes for vertex shader
- Make attributes automatically derive from shader; open up API for specifying additional attributes (see vertex-desc branch per Hei)
- When an item is removed from memory, it's not being correctly removed from the cache

## In-Progress


## Done

- ~~**(Editor)** Draw bounding volumes for rooms~~
- ~~Model import from SMD/QC etc.~~
- ~~Test the rope physics and confirm they're working~~
- ~~Introduce a way for the game to draw debug crap more *cleanly*, maybe some sort of deferred drawing API for basic wireframe primitives~~
- ~~Per-vertex lighting shader~~
- ~~Shadows from multiple sources don't work!~~
- ~~Add 'list_worlds' command~~
- ~~Add basic menu~~
- ~~Migrate rope physics from Doom 3~~
- ~~Versioning for binary node format~~
- ~~Remove class API per brushes; we'll have just poly-brushes now~~
- ~~Bitshift protocol version~~
- ~~Disconnect currently just crashes~~
- ~~Terrain brush type using heightmap with variable height specific textures~~
- ~~Fix post-processing, again~~
- ~~Allow for reloading shaders on command (hot-reload feature?)~~
- ~~Icons for nodes within viewport~~
  - ~~Get icons commissioned~~
- ~~Lensflare effect~~
- ~~Fix supersampling~~
- ~~Implement plotting for brushes~~
- ~~Colour selection buffer implemented in editor.c, use it to determine grid segment~~
  - ~~Draw large rectangles around grid intersections~~
- ~~Add a close option to 'File' menu which should just close the currently open editor~~
- ~~Allow us to hide the console. Make it the default?~~
- ~~Fix post-processing FBO fucking up general rendering~~
- ~~Fix issues with setting up viewports—why do we need to setup a dummy viewport??~~
- ~~Allow us to recurse up the tree to determine what room we're currently in—this is where draw should begin!
  Should be relative to camera~~
- ~~If no room, draw everything~~
- ~~Add a reset camera option to the viewport context menu~~
