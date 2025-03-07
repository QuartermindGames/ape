// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: A marker indicating where a player can spawn.
// Author:  Mark E. Sowden

#include "../game_private.h"

typedef struct PlayerSpawnEntity
{
	UInt team;
} PlayerSpawnEntity;
#define PLAYER_SPAWN_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), PlayerSpawnEntity )

static void *create_player_spawn( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( PlayerSpawnEntity );
}

static void spawn_player_spawn( ApeEntity *self )
{
}

static void destroy_player_spawn( ApeEntity *self )
{
}

ApeEntityClassDefinition game_playerSpawnEntityClass_ = {
        .name            = "player_spawn",
        .description     = "Creates a marker indicating where the player can spawn.",
        .createFunction  = create_player_spawn,
        .destroyFunction = destroy_player_spawn,
        .spawnFunction   = spawn_player_spawn,
};
