// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: World simulation state.

#include "detox_game.h"
#include "detox_world.h"

static ApeWorld *world;

void toxWorld_Spawn( void )
{
	world = apeGetCurrentWorld();
}

void toxWorld_Tick( void )
{
}
