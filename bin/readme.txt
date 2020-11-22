=================================================================
Project Yin
	Written by Mark "hogsy" Sowden (markelswo@gmail.com)
=================================================================

What is this?
-----------------------------------------------------------------
	This is a simplistic 3D engine that I've been developing
	on and off for a while now in C. 

	What you're looking at is obviously a work-in-progress.

	OSLauncherSDL2 - SDL2 input, windowing and other OS tasks
	OSEngine       - The engine itself, Yin
	OSCore         - Crap shared between everything
	pkgman         - Used to generate .pkg files (most files are automatically compressed)
	qmap2world     - Converts a .map file (exported from J.A.C.K.) to Yin's .wld format

What can I do with it?
-----------------------------------------------------------------
	Currently, not much. You could use J.A.C.K. to export some
	of your own maps and convert them, but right now you will
	have mixed results until Yin's editor is finished.

	I don't currently have plans to make the source code for
	the engine available, however I do plan on supporting Lua
	for scripting - when that arrives you will be able to do
	lots more!

Technical
-----------------------------------------------------------------

WLD
	Levels in Yin or "worlds" as they're referred to are
	hierarchical, every brush can have a child entity/brush and
	every entity could have a child entity/brush - there
	are no limitations (hell you could have an entire sector as a 
	child to an entity).

	Everything is then transformed in relation to the
	transform of it's parent. This allows you to have rooms
	that can be rotated with the player being rotated within
	and other crazy situations.

	There are a collection of head nodes which could be
	interpreted as sectors/rooms - the player will need
	to spawn within one of these in order to actually be
	able to navigate the world otherwise the engine will
	not be able to determine where they are.

	It's worth mentioning that every single room has it's
	own local space as well that everything is relative to.