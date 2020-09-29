/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "actor.h"

typedef struct ATriggerVolume {
	bool removeOnTrigger;
} ATriggerVolume;

void TriggerVolume_Spawn( Actor *self ) {
	ATriggerVolume *triggerData = Sys_calloc( 1, sizeof( ATriggerVolume ) );
	Act_SetUserData( self, triggerData );
}

void TriggerVolume_Collide( Actor *self, Actor *other, void *userData ) {
	ATriggerVolume *triggerData = ( ATriggerVolume * ) userData;

	/* if we're flagged for destruction, go ahead once we're done */
	if ( triggerData->removeOnTrigger ) {
		Act_DestroyActor( self );
	}
}
