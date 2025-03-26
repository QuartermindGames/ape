// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: A large floating airship that floats around the map.
// Author:  Mark E. Sowden

#include "../ss1_game.h"

#include "../../shared/components/component_health.h"

#include <ape/ape_public_model.h>

static constexpr float MIN_HEIGHT = 4096.0f;
static constexpr float MAX_RANGE  = 10000.0f;
static constexpr float MAX_SPEED  = 100.0f;

static constexpr unsigned int MAX_HEALTH = 200;

#define SS1_AIRSHIP_CLASS_NAME "ss1_airship"

typedef struct AirshipEntity
{
	GameHealthComponent *healthComponent;

	PLVector3 oldPosition;
	PLVector3 targetDestination;

	ApeAudioSample *engineSample;
	ApeAudioSource *audioSource;

	ApeModelNode *model;
} AirshipEntity;
#define AIRSHIP_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), SS1_AIRSHIP_CLASS_NAME, AirshipEntity )

static void cache_airship()
{
	//TODO: cache sound
}

static void *create_airship( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( AirshipEntity );
}

static void destroy_airship( ApeEntity *self )
{
	AirshipEntity *airship = AIRSHIP_ENTITY( self );
	assert( airship != nullptr );

	ape_audio_source_destroy( airship->audioSource );
	ape_audio_sample_release( airship->engineSample );

	PL_DELETE( airship );
}

static void update_target_destination( AirshipEntity *self )
{
	float x                 = PlGenerateRandomFloat( MAX_RANGE ) - PlGenerateRandomFloat( MAX_RANGE );
	float z                 = PlGenerateRandomFloat( MAX_RANGE ) - PlGenerateRandomFloat( MAX_RANGE );
	self->targetDestination = PL_VECTOR3( x, MIN_HEIGHT + PlGenerateRandomFloat( 100.0f ), z );
}

static void spawn_airship( ApeEntity *self )
{
	AirshipEntity *airship = AIRSHIP_ENTITY( self );

	airship->healthComponent = ape_entity_add_component( self, "health" );
	assert( airship->healthComponent != nullptr );
	airship->healthComponent->maxHealth = MAX_HEALTH;
	airship->healthComponent->health    = airship->healthComponent->maxHealth;

	airship->model = ape_model_node_create( APE_WORLD_NODE( self ), "airship_body", "models/airship.mdl.n" );
	ape_world_node_set_position( APE_WORLD_NODE( self ), &PL_VECTOR3( 0.0f, MIN_HEIGHT, 0.0f ) );

	airship->audioSource  = ape_audio_source_create( &PL_VECTOR3( 0.0f, MIN_HEIGHT, 0.0f ), &pl_vecOrigin3, APE_AUDIO_SOURCE_GROUP_GENERIC );
	airship->engineSample = ape_audio_sample_cache( "sounds/airship/engine.wav" );

	update_target_destination( airship );
}

static void tick_airship( ApeEntity *self, double delta )
{
	delta = game_get_time_delta_( delta );

	AirshipEntity *airship = AIRSHIP_ENTITY( self );

	PLVector3 pos = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );
	PLVector3 ang = ape_world_node_get_angles( APE_WORLD_NODE( self ) );

	airship->oldPosition = pos;

	PLVector3 direction = PL_VECTOR3( airship->targetDestination.x - pos.x, 0.0f, airship->targetDestination.z - pos.z );
	float     distance  = PlVector3Length( direction );
	if ( distance > 0.0f )
	{
		direction  = PlNormalizeVector3( direction );
		float step = MAX_SPEED * delta;

		if ( distance <= step )
		{
			pos.x = airship->targetDestination.x;
			pos.z = airship->targetDestination.z;
		}
		else
		{
			pos.x += direction.x * step;
			pos.z += direction.z * step;
		}

		ang.y = PL_RAD2DEG( atan2f( direction.z, direction.x ) );
	}

	// apply some bobbing to make the ship look floaty
	pos.y = MIN_HEIGHT + sinf( ape_get_num_ticks() / 80.0f ) / 10.0f * 1000.0f;
	ang.x = sinf( ape_get_num_ticks() / 1000.0f ) * 4.0f;
	ang.z = sinf( ape_get_num_ticks() / 1000.0f ) * 4.0f;

	ape_world_node_set_position( APE_WORLD_NODE( self ), &pos );
	ape_world_node_set_angles( APE_WORLD_NODE( self ), &ang );

	PLVector3 velocity = PlSubtractVector3( airship->oldPosition, pos );
	ape_audio_source_set_position( airship->audioSource, &pos );
	ape_audio_source_set_velocity( airship->audioSource, &velocity );

	if ( distance <= 0.0f )
	{
		update_target_destination( airship );
	}
}

ApeEntityClassDefinition ss1_airshipEntityClass = {
        .name        = SS1_AIRSHIP_CLASS_NAME,
        .description = "Decorative airship that can be blown up. It's very big!",

        .cacheFunction   = cache_airship,
        .createFunction  = create_airship,
        .destroyFunction = destroy_airship,
        .spawnFunction   = spawn_airship,
        .tickFunction    = tick_airship,
};
