/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "act.h"
#include "gfx.h"

static const char *walkFrameNames[] = {
			"BOSSA1", "BOSSA2", "BOSSA3", "BOSSA4", "BOSSA5", "BOSSA6", "BOSSA7", "BOSSA8",
			"BOSSB1", "BOSSB2", "BOSSB3", "BOSSB4", "BOSSB5", "BOSSB6", "BOSSB7", "BOSSB8",
			"BOSSC1", "BOSSC2", "BOSSC3", "BOSSC4", "BOSSC5", "BOSSC6", "BOSSC7", "BOSSC8",
			"BOSSD1", "BOSSD2", "BOSSD3", "BOSSD4", "BOSSD5", "BOSSD6", "BOSSD7", "BOSSD8",
};

#define BOSS_NUM_WALK_FRAMES plArrayElements( walkFrameNames )
#define BOSS_NUM_WALK_SETS   BOSS_NUM_WALK_FRAMES / GFX_NUM_SPRITE_ANGLES

typedef struct ABoss {
	GfxAnimationFrame *walkFrames[ BOSS_NUM_WALK_FRAMES ];
} ABoss;

void Boss_Draw( Actor *self, void *userData ) {}

void Boss_Tick( Actor *self, void *userData ) {
	/* wedge this in here, running out of time */
	static unsigned int frameDelay = 0;
	if( ++frameDelay > 32 ) {
		unsigned int curFrame = Act_GetCurrentFrame( self ) + 1;
		if( curFrame >= BOSS_NUM_WALK_SETS ) {
			curFrame = 0;
		}

		Act_SetCurrentFrame( self, curFrame );

		frameDelay = 0;
	}
}

void Boss_Spawn( Actor *self ) {
	ABoss *bossData = Sys_AllocateMemory( 1, sizeof( ABoss ) );
	Act_SetUserData( self, bossData );

	/* now to load in our sprite data... */
	
	Gfx_LoadAnimationFrames( walkFrameNames, bossData->walkFrames, BOSS_NUM_WALK_FRAMES );
}
