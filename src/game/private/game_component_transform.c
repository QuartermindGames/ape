// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "game_private.h"
#include "game_component_transform.h"

static void Spawn( ApeEntityComponent *self )
{
	self->userData = PL_NEW( ECTransform );
}

static NdBranch *Serialize( ApeEntityComponent *self, NdBranch *root )
{
	ndPushBackF32Array( root, "translation", ( float * ) &ECTRANSFORM( self )->translation, 3 );
	ndPushBackF32Array( root, "scale", ( float * ) &ECTRANSFORM( self )->scale, 3 );
	ndPushBackF32Array( root, "angles", ( float * ) &ECTRANSFORM( self )->angles, 3 );
	ndPushBackI32( root, "sectorNum", ECTRANSFORM( self )->sectorNum );
	return root;
}

static NdBranch *Deserialize( ApeEntityComponent *self, NdBranch *root )
{
	NdBranch *child;
	if ( ( child = ndGetChildByName( root, "translation" ) ) != NULL )
	{
		ndGetF32Array( child, ( float * ) &ECTRANSFORM( self )->translation, 3 );
	}
	if ( ( child = ndGetChildByName( root, "scale" ) ) != NULL )
	{
		ndGetF32Array( child, ( float * ) &ECTRANSFORM( self )->scale, 3 );
	}
	if ( ( child = ndGetChildByName( root, "angles" ) ) != NULL )
	{
		ndGetF32Array( child, ( float * ) &ECTRANSFORM( self )->angles, 3 );
	}
	ECTRANSFORM( self )->sectorNum = ndGetInt( root, "sectorNum", -1 );
	return root;
}

static void Tick( ApeEntityComponent *self )
{
	// if we're in the world, ensure we're attached to a valid sector
	ApeWorld *world = apeGetCurrentWorld();
	if ( world != NULL && ECTRANSFORM( self )->sectorNum == -1 )
	{
		Game_Warning( "Entity outside of world, attempting to relocate!\n" );

		ApeWorldRoom *sector = apeGetRoomAtPosition( world, &ECTRANSFORM( self )->translation );
		if ( sector != NULL )
		{
			//TODO: what fucking index is it!?
		}
		else
		{
			Game_Warning( "Failed to fetch sector by origin - falling to first sector!\n" );
			/*sector = World_GetSectorByNum( world, 0 );
			if ( sector == NULL )
			{
				Game_Warning( "No first sector, panic!!\n" );
			}*/
		}
	}
}

const ApeEntityComponentCallbackTable *EntityComponent_Transform_GetCallbackTable( void )
{
	static ApeEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );

	callbackTable.spawnFunction       = Spawn;
	callbackTable.serializeFunction   = Serialize;
	callbackTable.deserializeFunction = Deserialize;
	callbackTable.tickFunction        = Tick;

	return &callbackTable;
}
