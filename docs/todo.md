
### Todo
- Implement new tab type with close button so we can easily close editors
- Add a close option to 'File' menu which should just close the currently open editor
- Colour selection buffer implemented in editor.c, use it to determine grid segment
  - Draw large rectangles around grid intersections

### Done
- Allow us to recurse up the tree to determine what room we're currently in - this is where draw should begin! Should be relative to camera
- If no room, draw everything
- Add reset camera option to viewport context menu
