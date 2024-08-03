# APE 4 Materials

## Goals

- Add more explicit control to both materials and shaders so they can define more behaviours depending on certain passes being drawn, such as stencil shadow volumes, alpha passes and more
- Introduce a way for the game to fetch / modify variables that are set for a particular material (Source Engine has a similar feature, need to see how that's done); this will be handy for TOD behaviour in SS1 and SS2
- Experiment with ways we can make the shaders/materials more efficient (see [LOD section](#lod-management))
- Materials need to be simplified; need a proxy for variables that can be specified within the shader that materials can utilise instead, which would greatly simplify their design
- Try to avoid breaking compatility with existing materials

## Shaders

~~The type of object for shaders will be changed from `program` to `shader` for better clarity, and to differentiate them from the old material system.~~

`defaultPass` should be followed up with multiple other types of passes, depending on what the engine needs.

For instance, we should introduce a new `stencilShadowPass` which is used when rendering a surface as per the stencil shadow volumes.

### Passes / Techniques

- `stencilShadowPass`
- `alphaPass`

### LOD Management

This isn't something I've seen done elsewhere surprisingly, but it seems like it might be a good idea. Some pixel
operations are less apparent at a distance. So it might be an idea to allow materials some way of handling this.

We should ideally have different thresholds, either based on distance but we could allow for density to be considered
too, and then based on that provide fallbacks - maybe to less intensive shaders (or just pixel shaders)?

```
array object lods
{
	{
		; this determines when it'll kick in
		float threshold 1024
		
		...
	}
}
```

## Materials

Materials will be simplified, and instead the user is encouraged to utilise the global material scope, rather than
introducing their own individual passes via the passes array.

Perhaps we should consider ways to remain backwards compatible?
