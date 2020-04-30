/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "act.h"
#include "gfx.h"

static const char *walkFrameNames[] = {
			"SARGA1", "SARGA2", "SARGA3", "SARGA4", "SARGA5", "SARGA6", "SARGA7", "SARGA8",
			"SARGB1", "SARGB2", "SARGB3", "SARGB4", "SARGB5", "SARGB6", "SARGB7", "SARGB8",
			"SARGC1", "SARGC2", "SARGC3", "SARGC4", "SARGC5", "SARGC6", "SARGC7", "SARGC8",
			"SARGD1", "SARGD2", "SARGD3", "SARGD4", "SARGD5", "SARGD6", "SARGD7", "SARGD8",
};

#define SARG_NUM_WALK_FRAMES plArrayElements( walkFrameNames )
#define SARG_NUM_WALK_SETS   SARG_NUM_WALK_FRAMES / GFX_NUM_SPRITE_ANGLES

typedef struct ASarg {
	GfxAnimationFrame *walkFrames[ SARG_NUM_WALK_FRAMES ];
} ASarg;

void Sarg_Draw( Actor *self, void *userData ) {}

void Sarg_Tick( Actor *self, void *userData ) {
	/* wedge this in here, running out of time */
	static unsigned int frameDelay = 0;
	if( ++frameDelay > 32 ) {
		unsigned int curFrame = Act_GetCurrentFrame( self ) + 1;
		if( curFrame >= SARG_NUM_WALK_SETS ) {
			curFrame = 0;
		}

		Act_SetCurrentFrame( self, curFrame );

		frameDelay = 0;
	}
}

void Sarg_Spawn( Actor *self ) {
	ASarg *sargData = Sys_AllocateMemory( 1, sizeof( ASarg ) );
	Act_SetUserData( self, sargData );

	/* now to load in our sprite data... */
	Gfx_LoadAnimationFrames( walkFrameNames, sargData->walkFrames, SARG_NUM_WALK_FRAMES );
}
