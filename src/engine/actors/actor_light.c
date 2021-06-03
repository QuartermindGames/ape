/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "actor_light.h"

void Light_Spawn( Actor *self )
{
	ALight *lightData = globalSystem.MAlloc( sizeof( ALight ), true );
	Act_SetUserData( self, lightData );

	/* for now, just randomise the light colours */
	lightData->colour = PLColourRGB( rand() % 255, rand() % 255, rand() % 255 );
	lightData->type   = PLG_LIGHT_TYPE_OMNI;
}

void Light_Tick( Actor *self, void *userData )
{
}

/**
 * Pretty much just for debugging.
 */
void Light_Draw( Actor *self, void *userData )
{
}
