// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Players - we play as 'em!
// Author:  Mark E. Sowden

#include "../ss1_game.h"

#include <ape/ape_public_model.h>

#include "entity_player.h"

#include "../../shared/components/component_health.h"
#include "../../shared/components/component_movement.h"

static void *create_player_entity( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( SS1PlayerEntity );
}

static void spawn_player_entity( ApeEntity *self )
{
	SS1PlayerEntity *player = SS1_PLAYER_ENTITY( self );

	// just randomize the initial profession for now
	player->profession = rand() % SS1_MAX_PROFESSIONS;

	player->movementComponent = ape_entity_add_component( self, "movement" );
	//assert( player->movementComponent != nullptr );

	player->healthComponent = ape_entity_add_component( self, "health" );
	assert( player->healthComponent != nullptr );
	player->healthComponent->maxHealth = ss1_professions[ player->profession ].maxHealth;
	player->healthComponent->health    = player->healthComponent->maxHealth;

	PLVector3 pos = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );
	for ( unsigned int i = 0; i < SS1_PLAYER_MAX_AUDIO_CHANNELS; ++i )
	{
		player->audioSources[ i ] = ape_audio_source_create( &pos, &pl_vecOrigin3, APE_AUDIO_SOURCE_GROUP_GENERIC );
	}

	player->model = ape_model_node_create( APE_WORLD_NODE( self ), "player_body", "models/characters/character_test.mdl.n" );
	assert( player->model != nullptr );
}

ApeEntityClassDefinition ss1_playerEntityClass = {
        .name        = "ss1_player",
        .description = "Player entity. This shouldn't be placed directly!",

        .createFunction = create_player_entity,
        .spawnFunction  = spawn_player_entity,
};
