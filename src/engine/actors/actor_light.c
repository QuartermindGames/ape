/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com> */

#include "yin.h"
#include "actor_light.h"

void Light_Spawn( Actor *self ) {
	ALight *lightData = Sys_calloc( 1, sizeof( ALight ) );
	Act_SetUserData( self, lightData );

	/* for now, just randomise the light colours */
	lightData->colour = PLColourRGB( rand() % 255, rand() % 255, rand() % 255 );
	lightData->type = PL_LIGHT_TYPE_OMNI;
}

void Light_Tick( Actor *self, void *userData ) {

}

/**
 * Pretty much just for debugging.
 */
void Light_Draw( Actor *self, void *userData ) {

}
