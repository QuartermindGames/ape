// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "game_component_mesh.h"
#include "game_component_transform.h"

static void Spawn( ApeEntityComponent *self )
{
	self->userData = PL_NEW( GameMeshComponent );

	GAME_MESH_COMPONENT( self )->transformComponent = apeGetEntityComponentByName( self->entity, "transform" );

	GAME_MESH_COMPONENT( self )->mesh = PlgCreateMesh( PLG_MESH_LINES, PLG_DRAW_STATIC, 0, 6 );
	GAME_MESH_COMPONENT( self )->material = apeCacheMaterial( "engine/vertex.mat.n", APE_CACHE_WORLD, true, false );
}

static void Destroy( ApeEntityComponent *self )
{
	apeReleaseMaterial( GAME_MESH_COMPONENT( self )->material );

	PlgDestroyMesh( GAME_MESH_COMPONENT( self )->mesh );
}

static void Tick( ApeEntityComponent *self )
{
	Game_Print( "TICK\n" );
}

static void Draw( ApeEntityComponent *self )
{
	apeDrawMesh( GAME_MESH_COMPONENT( self )->material, GAME_MESH_COMPONENT( self )->mesh, NULL, 0 );
}

const ApeEntityComponentCallbackTable *gameMeshComponentCallbackTable( void )
{
	static ApeEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );
	callbackTable.spawnFunction = Spawn;
	callbackTable.destroyFunction = Destroy;
	callbackTable.drawFunction = Draw;

	return &callbackTable;
}
