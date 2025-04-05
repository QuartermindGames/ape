// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Players - we play as 'em!
// Author:  Mark E. Sowden

#include "../ss1_game.h"

#include <ape/ape_public_model.h>

#include "entity_player.h"

#include "../../shared/components/component_health.h"
#include "../../shared/components/component_collision.h"
#include "../../shared/components/component_movement.h"

static constexpr float PLAYER_CAMERA_HEIGHT   = 45.0f;
static constexpr float PLAYER_CAMERA_DISTANCE = 50.0f;
static constexpr float PLAYER_CAMERA_SIDE     = 10.0f;

static void *create_player_entity( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( SS1PlayerEntity );
}

static void setup_default_equipment( ApeEntity *self )
{
	SS1PlayerEntity *player = SS1_PLAYER_ENTITY( self );
	assert( player != nullptr );

	switch ( player->profession )
	{
		default:
			//TODO: print name of player... this isn't exposed yet
			game_warning_( "Player doesn't have a valid profession!\n" );
			break;
		case SS1_PROFESSION_SHAMAN: break;
		case SS1_PROFESSION_MACHINIST: break;
		case SS1_PROFESSION_TRICKSTER: break;
		case SS1_PROFESSION_POUNDER: break;
	}
}

static void spawn_player_entity( ApeEntity *self )
{
	SS1PlayerEntity *player = SS1_PLAYER_ENTITY( self );
	assert( player != nullptr );

	PLVector3 pos = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );
	PLVector3 ang = ape_world_node_get_angles( APE_WORLD_NODE( self ) );

	// just randomize the initial profession for now
	player->profession = rand() % SS1_MAX_PROFESSIONS;

	player->movementComponent = ape_entity_add_component( self, "movement" );
	assert( player->movementComponent != nullptr );

	player->healthComponent = ape_entity_add_component( self, "health" );
	assert( player->healthComponent != nullptr );
	player->healthComponent->maxHealth = ss1_professions[ player->profession ].maxHealth;
	player->healthComponent->health    = player->healthComponent->maxHealth;

	player->collisionComponent = ape_entity_add_component( self, "collision" );
	assert( player->collisionComponent != nullptr );
	player->collisionComponent->type = APE_COLLISION_TYPE_AABB;

	player->inventoryComponent = ape_entity_add_component( self, "inventory" );
	assert( player->inventoryComponent != nullptr );

	for ( unsigned int i = 0; i < SS1_PLAYER_MAX_AUDIO_CHANNELS; ++i )
	{
		player->audioSources[ i ] = ape_audio_source_create( &pos, &pl_vecOrigin3, APE_AUDIO_SOURCE_GROUP_GENERIC );
	}

	player->model = ape_model_node_create( APE_WORLD_NODE( self ), "player_body", "models/characters/character_test.mdl.n" );
	assert( player->model != nullptr );

	// setup the camera state
	player->cameraHeight   = PLAYER_CAMERA_HEIGHT;
	player->cameraDistance = PLAYER_CAMERA_DISTANCE;
	player->cameraSide     = PLAYER_CAMERA_SIDE;

	PLVector3 forward;
	PlAnglesAxes( ang, nullptr, nullptr, &forward );
	forward              = PlNormalizeVector3( forward );
	player->cameraAngles = PL_VECTOR3( 0.0f, PL_RAD2DEG( atan2f( forward.x, forward.z ) ) + 180.0f, 0.0f );

	//TODO: this shouldn't be here...
	ss1_gameState.cameraState = SS1_CAMERA_STATE_THIRD_PERSON;

	setup_default_equipment( self );
}

static void tick_player_entity( ApeEntity *self, double delta )
{
	delta = game_get_time_delta_( delta );

	SS1PlayerEntity *player = SS1_PLAYER_ENTITY( self );
	assert( player != nullptr );

	game_component_movement_tick_( player->movementComponent, self, delta );
}

ApeEntityClassDefinition ss1_playerEntityClass = {
        .name        = SS1_PLAYER_CLASS_NAME,
        .description = "Player entity. This shouldn't be placed directly!",

        .createFunction = create_player_entity,
        .spawnFunction  = spawn_player_entity,

        .tickFunction = tick_player_entity,
};
