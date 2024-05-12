# Todo

## Future
- Initial Lua mock-up
- Investigate a solution for giving stencil shadows a softer appearance
  - One I've seen is to essentially draw the result of the stencil shadow volumes to another buffer, blur it and then apply it to the scene; if we include depth information with this, we could use it for controlling the samples per depth (kind of like a DOF effect)
- Look into adding support for DOF
- Stencil shadow volumes only need to extend within the sphere/radius of a local light source
- Look into light-bleeding issue...
- Restrict vertices for stencil shadows based on point of intersection
  - If point doesn't cross/intersect, we should have a reasonable limit

## Current
- Mirrors—correctly traverse from room to build a visible list of rooms w/ transforms
- Shadows from multiple sources don't work!
- We don't have spotlights...
- Prototype 'eyes' for SS2
  - Plane with two layers, the second layer representing pupil will take offset for eye movement. Maybe the first layer acts as a mask?
  - Add some logic for swapping out first texture, so we can convey different expressions
- Implement DetachShaderStage in graphics driver
- Allow for reloading materials on command
- Process a level directly from a .map rather than from an .obj, and process the entities
  - Process entities from a .map
- World deserialiser should perform deserialisation on context of a node tree, rather than explicitly by type
- 'current' world selection in editor
- **(Editor)** Different viewport implementations, as some logic for viewport isn't necessary for others
- **(Editor)** Draw bounding volumes for rooms
- **(Editor)** Add texture selection frame
- Implement a new tab type with close button, so we can easily close editors
- **(Editor)** Editor is crashing when attempting to close an editor tab (looks like an issue with queued job?)
- **(Editor)** Look into the concept of supporting multiple grids
- **(Editor)** Make the console correctly hide/show, and resize with a window
- Terrain brush type using heightmap with variable height specific textures
- Sky plane should display irrelevent of camera position

## In-Progress
- Model import from SMD/QC etc.

## Done
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
