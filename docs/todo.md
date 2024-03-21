# Todo

## Future
- Restrict vertices for stencil shadows based on point of intersection
  - If point doesn't cross/intersect, we should have a reasonable limit

## Current
- Different viewport implementations, as some logic for viewport isn't necessary for others
- Draw bounding volumes for rooms
- Add texture selection frame
- Implement a new tab type with close button, so we can easily close editors
- Editor is crashing when attempting to close an editor tab (looks like an issue with queued job?)
- Look into the concept of supporting multiple grids
- Make the console correctly hide/show, and resize with a window

## In-Progress
- Fix supersampling
- Icons for nodes within viewport
  - ~~Get icons commissioned~~

## Done
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
