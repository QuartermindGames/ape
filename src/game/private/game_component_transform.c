// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "game_private.h"
#include "game_component_transform.h"

static void Spawn( YNCoreEntityComponent *self )
{
	self->userData = PL_NEW( ECTransform );
}

static YNNodeBranch *Serialize( YNCoreEntityComponent *self, YNNodeBranch *root )
{
	YnNode_PushBackF32Array( root, "translation", ( float * ) &ECTRANSFORM( self )->translation, 3 );
	YnNode_PushBackF32Array( root, "scale", ( float * ) &ECTRANSFORM( self )->scale, 3 );
	YnNode_PushBackF32Array( root, "angles", ( float * ) &ECTRANSFORM( self )->angles, 3 );
	YnNode_PushBackI32( root, "sectorNum", ECTRANSFORM( self )->sectorNum );
	return root;
}

static YNNodeBranch *Deserialize( YNCoreEntityComponent *self, YNNodeBranch *root )
{
	YNNodeBranch *child;
	if ( ( child = YnNode_GetChildByName( root, "translation" ) ) != NULL )
	{
		YnNode_GetF32Array( child, ( float * ) &ECTRANSFORM( self )->translation, 3 );
	}
	if ( ( child = YnNode_GetChildByName( root, "scale" ) ) != NULL )
	{
		YnNode_GetF32Array( child, ( float * ) &ECTRANSFORM( self )->scale, 3 );
	}
	if ( ( child = YnNode_GetChildByName( root, "angles" ) ) != NULL )
	{
		YnNode_GetF32Array( child, ( float * ) &ECTRANSFORM( self )->angles, 3 );
	}
	ECTRANSFORM( self )->sectorNum = YnNode_GetI32ByName( root, "sectorNum", -1 );
	return root;
}

static void Tick( YNCoreEntityComponent *self )
{
	// if we're in the world, ensure we're attached to a valid sector
	OgeWorld *world = Game_GetCurrentWorld();
	if ( world != NULL && ECTRANSFORM( self )->sectorNum == -1 )
	{
		Game_Warning( "Entity outside of world, attempting to relocate!\n" );

		OgeWorldSector *sector = YnCore_World_GetSectorByGlobalOrigin( world, &ECTRANSFORM( self )->translation );
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

const YNCoreEntityComponentCallbackTable *EntityComponent_Transform_GetCallbackTable( void )
{
	static YNCoreEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );

	callbackTable.spawnFunction       = Spawn;
	callbackTable.serializeFunction   = Serialize;
	callbackTable.deserializeFunction = Deserialize;
	callbackTable.tickFunction        = Tick;

	return &callbackTable;
}
