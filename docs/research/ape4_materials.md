# APE 4 Materials

## Shaders

The type of object for shaders will be changed from `program` to `shader` for better clarity, and to differentiate them from the old material system.

`defaultPass` should be followed up with multiple other types of passes, depending on what the engine needs.

For instance, we should introduce a new `stencilShadowPass` which is used when rendering a surface as per the stencil shadow volumes.

## Materials

Materials will be simplified and instead the user is encouraged to utilise the global material scope, rather than introducing their own individual passes via the passes array.

Perhaps we should consider ways to remain backwards compatible?

We will need to introduce a migration pass for old materials, fortunately we can 
