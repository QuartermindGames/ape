### Todo

- Implement plotting for brushes
- Draw bounding volumes for rooms
- Implement new tab type with close button so we can easily close editors
- Editor is crashing when attempting to close an editor
- Look into the concept of supporting multiple grids
- Colour selection buffer implemented in editor.c, use it to determine grid segment
    - **done**: Draw large rectangles around grid intersections
- Make the console correctly hide/show, and resize with window

----

- **done**: Add a close option to 'File' menu which should just close the currently open editor
- **done**: Allow us to hide the console. Make it the default?
- **done**: Fix post-processing FBO fucking up general rendering
- **done**: Fix issues with setting up viewports - why do we need to setup a dummy viewport??
- **done**: Allow us to recurse up the tree to determine what room we're currently in - this is where draw should begin!
  Should be relative to camera
- **done**: If no room, draw everything
- **done**: Add reset camera option to viewport context menu
