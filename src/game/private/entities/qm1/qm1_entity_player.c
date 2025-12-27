// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Players - we play as 'em!
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_random.h"

#include "ss1/ss1_game.h"

#include "core/public/ape/ape_public_model.h"

#include "qm1_entity_player.h"

#include "components/component_health.h"
#include "components/component_collision.h"
#include "components/component_movement.h"
#include "physics/physics.h"
#include "game_entity.h"

static constexpr float PLAYER_CAMERA_HEIGHT   = 45.0f;
static constexpr float PLAYER_CAMERA_DISTANCE = 50.0f;
static constexpr float PLAYER_CAMERA_SIDE     = 10.0f;

static void *create_player_entity( ApeEntity *self, AcmBranch *properties )
{
	return QM_OS_MEMORY_NEW( SS1PlayerEntity );
}

//static void setup_default_equipment( ApeEntity *self )
//{
//	SS1PlayerEntity *player = SS1_PLAYER_ENTITY( self );
//	assert( player != nullptr );
//
//	switch ( player->profession )
//	{
//		default:
//			//TODO: print name of player... this isn't exposed yet
//			game_warning_( "Player doesn't have a valid profession!\n" );
//			break;
//		case SS1_PROFESSION_SHAMAN: break;
//		case SS1_PROFESSION_MACHINIST: break;
//		case SS1_PROFESSION_TRICKSTER: break;
//		case SS1_PROFESSION_POUNDER: break;
//	}
//}

static void spawn_player_entity( ApeEntity *self )
{
	SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( self );
	assert( playerEntity != nullptr );

	QmMathVector3f pos = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );
	QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( self ) );

	// just randomize the initial profession for now
	//unsigned int seed  = qm_os_random_seed_initialize();
	//player->profession = qm_os_random_int( &seed ) % SS1_MAX_PROFESSIONS;

	playerEntity->movementComponent = ape_entity_add_component( self, "movement" );
	assert( playerEntity->movementComponent != nullptr );

	playerEntity->healthComponent = ape_entity_add_component( self, "health" );
	assert( playerEntity->healthComponent != nullptr );

	// setup collision component
	GameCollisionComponent *collisionComponent = ape_entity_add_component( self, "collision" );
	assert( collisionComponent != nullptr );
	collisionComponent->groups                 = GAME_COLLISION_GROUP_PLAYER;
	collisionComponent->type                   = APE_COLLISION_TYPE_SPHERE;//APE_COLLISION_TYPE_AABB;
	collisionComponent->collider.sphere.radius = 4.0f;
	collisionComponent->collider.sphere.origin = qm_math_vector3f( 0.0f, collisionComponent->collider.sphere.radius, 0.0f );
	playerEntity->collisionComponent           = collisionComponent;

	playerEntity->inventoryComponent = ape_entity_add_component( self, "inventory" );
	assert( playerEntity->inventoryComponent != nullptr );

	for ( unsigned int i = 0; i < SS1_PLAYER_MAX_AUDIO_CHANNELS; ++i )
	{
		playerEntity->audioSources[ i ] = ape_audio_source_create( &pos, &pl_vecOrigin3, APE_AUDIO_SOURCE_GROUP_GENERIC );
	}

	playerEntity->model = ape_model_node_create( APE_WORLD_NODE( self ), "player_body", "models/characters/character_test.mdl.n" );
	assert( playerEntity->model != nullptr );
	//APE_WORLD_NODE( playerEntity->model )->flags |= APE_WORLD_NODE_FLAG_HIDDEN;

	// setup the camera state
	playerEntity->cameraHeight   = PLAYER_CAMERA_HEIGHT;
	playerEntity->cameraDistance = PLAYER_CAMERA_DISTANCE;
	playerEntity->cameraSide     = PLAYER_CAMERA_SIDE;

	QmMathVector3f forward;
	PlAnglesAxes( ang, nullptr, nullptr, &forward );
	forward                    = qm_math_vector3f_normalize( forward );
	playerEntity->cameraAngles = qm_math_vector3f( 0.0f, QM_MATH_RAD2DEG( atan2f( forward.x, forward.z ) ) + 180.0f, 0.0f );

	//setup_default_equipment( self );
}

static void tick_player_entity( ApeEntity *self, double delta )
{
	return;
	delta = game_get_delta_mod_( delta );

	SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( self );
	assert( playerEntity != nullptr );

	game_component_movement_tick_( playerEntity->movementComponent, playerEntity->collisionComponent, self, delta );
}

void qm1_entity_player_assign( ApeEntity *self, GamePlayer *player, Qm1Character *character )
{
	SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( self );
	assert( playerEntity != nullptr );

	playerEntity->player    = player;
	playerEntity->character = character;
}

ApeEntityClassDefinition ss1_playerEntityClass = {
        .name        = SS1_PLAYER_CLASS_NAME,
        .description = "Player entity. This shouldn't be placed directly!",

        .createFunction = create_player_entity,
        .spawnFunction  = spawn_player_entity,

        .tickFunction = tick_player_entity,
};
