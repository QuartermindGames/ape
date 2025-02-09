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

typedef struct AirshipEntity
{
	GameHealthComponent *healthComponent;

	PLVector3 targetDestination;

	ApeAudioSource *source;
	ApeModelNode   *model;
} AirshipEntity;
#define AIRSHIP_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), AirshipEntity )

static void cache_airship()
{
	//TODO: cache sound
}

static void *create_airship( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( AirshipEntity );
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

	update_target_destination( airship );
}

static void tick_airship( ApeEntity *self, double delta )
{
	AirshipEntity *airship = AIRSHIP_ENTITY( self );

	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	PLVector3 ang = ape_world_node_get_angles( APE_WORLD_NODE( self ) );

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

		ang.y = atan2f( direction.z, direction.x ) * ( 180.0f / PL_PI );
	}

	// apply some bobbing to make the ship look floaty
	pos.y = MIN_HEIGHT + sinf( ape_get_num_ticks() / 80.0f ) / 10.0f * 1000.0f;

	ang.x = sinf( ape_get_num_ticks() / 1000.0f ) * 4.0f;
	ang.z = sinf( ape_get_num_ticks() / 1000.0f ) * 4.0f;

	ape_world_node_set_position( APE_WORLD_NODE( self ), &pos );
	ape_world_node_set_angles( APE_WORLD_NODE( self ), &ang );

	if ( distance <= 0.0f )
	{
		update_target_destination( airship );
	}
}

ApeEntityClassDefinition ss1_airshipEntityClass = {
        .name        = "ss1_airship",
        .description = "Decorative airship that can be blown up. It's very big!",

        .cacheFunction  = cache_airship,
        .createFunction = create_airship,
        .spawnFunction  = spawn_airship,
        .tickFunction   = tick_airship,
};
