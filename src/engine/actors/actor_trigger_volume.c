/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "actor.h"

typedef struct ATriggerVolume
{
	bool removeOnTrigger;
} ATriggerVolume;

void TriggerVolume_Spawn( Actor *self )
{
	ATriggerVolume *triggerData = globalSystem.MAlloc( sizeof( ATriggerVolume ), true );
	Act_SetUserData( self, triggerData );
}

void TriggerVolume_Collide( Actor *self, Actor *other, void *userData )
{
	ATriggerVolume *triggerData = ( ATriggerVolume * ) userData;

	/* if we're flagged for destruction, go ahead once we're done */
	if ( triggerData->removeOnTrigger )
	{
		Act_DestroyActor( self );
	}
}
