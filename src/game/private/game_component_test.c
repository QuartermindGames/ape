// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "game_component_test.h"
#include "game_component_transform.h"

static void Spawn( ApeEntityComponent *self ) {
	self->userData = PL_NEW( GameTestComponent );

	GAME_TEST_COMPONENT( self )->transformComponent = apeGetEntityComponentByName( self->entity, "transform" );
	if ( GAME_TEST_COMPONENT( self )->transformComponent != NULL ) {
	} else {
		Game_Warning( "No transform component for test entity!\n" );
	}
}

static void Tick( ApeEntityComponent *self ) {
}

const ApeEntityComponentCallbackTable *gameTestComponentCallbackTable( void ) {
	static ApeEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );

	callbackTable.spawnFunction = Spawn;
	callbackTable.tickFunction = Tick;

	return &callbackTable;
}
