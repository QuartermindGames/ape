// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Players - we play as 'em!
// Author:  Mark E. Sowden

#include "nihlexa/nihlexa.h"

#include "core/public/ape/ape_public_model.h"

#include "nih_entity_player.h"

#include "components/component_health.h"
#include "components/component_collision.h"
#include "components/component_movement.h"
#include "components/component_camera.h"

#include "physics/physics.h"

static constexpr float NIH_PLAYER_BASE_HEIGHT = 72.0f;
static constexpr float NIH_PLAYER_BASE_RADIUS = 16.0f;

static constexpr int16_t NIH_PLAYER_BASE_HEALTH = 100;
static constexpr int16_t NIH_PLAYER_MAX_HEALTH  = 200;

static constexpr float NIH_PLAYER_WALK_SPEED = 45.0f;
static constexpr float NIH_PLAYER_RUN_SPEED  = 75.0f;
static constexpr float NIH_PLAYER_JUMP_SPEED = 100.0f;

static void *create_player_entity( ApeEntity *self )
{
	NihPlayerEntity *player = QM_OS_MEMORY_NEW( NihPlayerEntity );

	GameMovementComponent *movement = ape_entity_add_component( self, "movement" );
	assert( movement != nullptr );
	movement->maxWalkSpeed = NIH_PLAYER_WALK_SPEED;
	movement->maxRunSpeed  = NIH_PLAYER_RUN_SPEED;
	movement->jumpSpeed    = NIH_PLAYER_JUMP_SPEED;
	movement->capabilities = GAME_MOVEMENT_CAPABILITY_CROUCH |
	                         GAME_MOVEMENT_CAPABILITY_CROUCH_MOVE |
	                         GAME_MOVEMENT_CAPABILITY_JUMP;
	player->movementComponent = movement;

	player->healthComponent = ape_entity_add_component( self, "health" );
	assert( player->healthComponent != nullptr );
	player->healthComponent->health    = NIH_PLAYER_BASE_HEALTH;
	player->healthComponent->maxHealth = NIH_PLAYER_MAX_HEALTH;

	// setup collision component
	player->collisionComponent = ape_entity_add_component( self, "collision" );
	assert( player->collisionComponent != nullptr );
	player->collisionComponent->groups                   = GAME_COLLISION_GROUP_PLAYER;
	player->collisionComponent->type                     = APE_COLLISION_TYPE_CYLINDER;
	player->collisionComponent->collider.cylinder.height = NIH_PLAYER_BASE_HEIGHT;
	player->collisionComponent->collider.cylinder.radius = NIH_PLAYER_BASE_RADIUS;
	player->collisionComponent->collider.cylinder.origin = ape_world_node_get_position( APE_WORLD_NODE( self ) );

	player->cameraComponent = ape_entity_add_component( self, "camera" );
	assert( player->cameraComponent != nullptr );

	player->model = ape_model_node_create( APE_WORLD_NODE( self ), "playerModel", "models/editor/cylinder.mdl.n" );
	ape_world_node_set_scale( APE_WORLD_NODE( player->model ), &QM_MATH_VECTOR3F( 16.0f, 32.0f, 16.0f ) );

	// reset camera to direction entity is facing
	QmMathVector3f forward          = ape_world_node_get_forward( APE_WORLD_NODE( self ) );
	player->cameraComponent->angles = qm_math_vector3f( 0.0f, QM_MATH_RAD2DEG( atan2f( forward.x, forward.z ) ), 0.0f );

	return player;
}

static void spawn_player_entity( ApeEntity *self )
{
	NihPlayerEntity *player = NIH_PLAYER_ENTITY( self );
	assert( player != nullptr );

	QmMathVector3f pos = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );
	for ( unsigned int i = 0; i < NIH_PLAYER_MAX_AUDIO_CHANNELS; ++i )
	{
		player->audioSources[ i ] = ape_audio_source_create( &pos, &QM_MATH_VECTOR3F_ZERO, APE_AUDIO_SOURCE_GROUP_GENERIC );
	}
}

static void tick_player_entity( ApeEntity *self, double delta )
{
	delta = game_get_delta_mod_( delta );

	NihPlayerEntity *playerEntity = NIH_PLAYER_ENTITY( self );
	assert( playerEntity != nullptr );

	GameCameraComponent *cameraComponent = playerEntity->cameraComponent;

	QmMathVector3f ang = QM_MATH_VECTOR3F( 0.0f, cameraComponent->angles.y, 0.0f );
	ape_world_node_set_angles( APE_WORLD_NODE( self ), &ang );

	//GameCollisionComponent *collisionComponent = playerEntity->collisionComponent;
	//ape_draw_debug_cylinder( &collisionComponent->collider.cylinder, &QM_MATH_COLOUR4UB_RGB( 1.0f, 1.0f, 1.0f ), 16 );

	game_component_movement_tick_( playerEntity->movementComponent, playerEntity->collisionComponent, self, delta );
}

void nih_entity_player_set_camera_state( const ApeEntity *self, GameCameraState state )
{
	const NihPlayerEntity *player = NIH_PLAYER_ENTITY( self );
	assert( player != nullptr );

	game_component_camera_set_state_( player->cameraComponent, state );

	ApeWorldNode *modelNode = APE_WORLD_NODE( player->model );
	if ( modelNode == nullptr )
	{
		return;
	}

	if ( state == GAME_CAMERA_STATE_FIRST_PERSON )
	{
		modelNode->flags |= APE_WORLD_NODE_FLAG_HIDDEN;
	}
	else
	{
		modelNode->flags &= ~APE_WORLD_NODE_FLAG_HIDDEN;
	}
}

void nih_entity_player_toggle_camera_state( const ApeEntity *self )
{
	const NihPlayerEntity *player = NIH_PLAYER_ENTITY( self );
	assert( player != nullptr );

	GameCameraState            newState;
	const GameCameraComponent *cameraComponent = player->cameraComponent;
	if ( cameraComponent->state == GAME_CAMERA_STATE_FIRST_PERSON )
	{
		newState = GAME_CAMERA_STATE_THIRD_PERSON;
	}
	else
	{
		newState = GAME_CAMERA_STATE_FIRST_PERSON;
	}

	nih_entity_player_set_camera_state( self, newState );
}

ApeEntityClassDefinition nih_playerEntityClass = {
        .name        = NIH_PLAYER_CLASS_NAME,
        .description = "Player entity. This shouldn't be placed directly!",

        .createFunction = create_player_entity,
        .spawnFunction  = spawn_player_entity,

        .tickFunction = tick_player_entity,
};
