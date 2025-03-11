// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: A marker indicating where a player can spawn.
// Author:  Mark E. Sowden

#include "../game_private.h"

#include "entity_player_spawn.h"

static PLLinkedList *playerSpawnPoints;

PLLinkedList *game_player_spawn_get_spawn_points()
{
	return playerSpawnPoints;
}

static void *create_player_spawn( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( GamePlayerSpawnEntity );
}

static void spawn_player_spawn( ApeEntity *self )
{
	if ( playerSpawnPoints == nullptr )
	{
		playerSpawnPoints = PlCreateLinkedList();
	}

	PLAYER_SPAWN_ENTITY( self )->listNode = PlInsertLinkedListNode( playerSpawnPoints, self );
}

static void destroy_player_spawn( ApeEntity *self )
{
	PlDestroyLinkedListNode( PLAYER_SPAWN_ENTITY( self )->listNode );
	if ( PlGetNumLinkedListNodes( playerSpawnPoints ) == 0 )
	{
		PlDestroyLinkedList( playerSpawnPoints );
		playerSpawnPoints = nullptr;
	}

	PL_DELETE( PLAYER_SPAWN_ENTITY( self ) );
}

ApeEntityClassDefinition game_playerSpawnEntityClass_ = {
        .name            = "player_spawn",
        .description     = "Creates a marker indicating where the player can spawn.",
        .createFunction  = create_player_spawn,
        .destroyFunction = destroy_player_spawn,
        .spawnFunction   = spawn_player_spawn,
};
