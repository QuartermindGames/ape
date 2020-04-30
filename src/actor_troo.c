/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "act.h"
#include "gfx.h"

static const char *walkFrameNames[] = {
			"TROOA1", "TROOA2", "TROOA3", "TROOA4", "TROOA5", "TROOA6", "TROOA7", "TROOA8",
			"TROOB1", "TROOB2", "TROOB3", "TROOB4", "TROOB5", "TROOB6", "TROOB7", "TROOB8",
			"TROOC1", "TROOC2", "TROOC3", "TROOC4", "TROOC5", "TROOC6", "TROOC7", "TROOC8",
			"TROOD1", "TROOD2", "TROOD3", "TROOD4", "TROOD5", "TROOD6", "TROOD7", "TROOD8",
};

#define TROO_NUM_WALK_FRAMES plArrayElements( walkFrameNames )
#define TROO_NUM_WALK_SETS   TROO_NUM_WALK_FRAMES / GFX_NUM_SPRITE_ANGLES

typedef struct ATroo {
	GfxAnimationFrame *walkFrames[ TROO_NUM_WALK_FRAMES ];
} ATroo;

void Troo_Draw( Actor *self, void *userData ) {}

void Troo_Tick( Actor *self, void *userData ) {
	/* wedge this in here, running out of time */
	static unsigned int frameDelay = 0;
	if( ++frameDelay > 32 ) {
		unsigned int curFrame = Act_GetCurrentFrame( self ) + 1;
		if( curFrame >= TROO_NUM_WALK_SETS ) {
			curFrame = 0;
		}

		Act_SetCurrentFrame( self, curFrame );

		frameDelay = 0;
	}
}

void Troo_Spawn( Actor *self ) {
	ATroo *trooData = Sys_AllocateMemory( 1, sizeof( ATroo ) );
	Act_SetUserData( self, trooData );

	/* now to load in our sprite data... */
	Gfx_LoadAnimationFrames( walkFrameNames, trooData->walkFrames, TROO_NUM_WALK_FRAMES );
}
