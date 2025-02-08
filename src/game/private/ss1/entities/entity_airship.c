// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Wiggly jiggly ropes!
// Author:  Mark E. Sowden

#include "../ss1_game.h"

#include "../../shared/components/component_health.h"

#include <ape/ape_public_model.h>

static constexpr float MAX_HEIGHT = 2000.0f;

typedef struct AirshipEntity
{
	GameHealthComponent *healthComponent;

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

static void spawn_airship( ApeEntity *self )
{
	AirshipEntity *airship = AIRSHIP_ENTITY( self );

	airship->healthComponent = ape_entity_add_component( self, "health" );

	airship->model = ape_model_node_create( APE_WORLD_NODE( self ), "airship_body", "models/airship.mdl.n" );
	ape_world_node_set_position( APE_WORLD_NODE( self ), &PL_VECTOR3( 0.0f, MAX_HEIGHT, 0.0f ) );
}

static void tick_airship( ApeEntity *self )
{
	PLVector3 pos = ape_world_node_get_position( APE_WORLD_NODE( self ) );
	pos.y         = MAX_HEIGHT + sinf( ape_get_num_ticks() / 80.0f ) / 10.0f * 1000.0f;
	ape_world_node_set_position( APE_WORLD_NODE( self ), &pos );

	PLVector3 ang = ape_world_node_get_angles( APE_WORLD_NODE( self ) );
	ang.x         = sinf( ape_get_num_ticks() / 1000.0f ) * 4.0f;
	ang.y         = sinf( ape_get_num_ticks() / 1000.0f ) * 4.0f;
	ang.z         = sinf( ape_get_num_ticks() / 1000.0f ) * 4.0f;
	ape_world_node_set_angles( APE_WORLD_NODE( self ), &ang );
}

ApeEntityClassDefinition ss1_airshipEntityClass = {
        .name        = "airship",
        .description = "Decorative airship that can be blown up. It's very big!",

        .cacheFunction  = cache_airship,
        .createFunction = create_airship,
        .spawnFunction  = spawn_airship,
        .tickFunction   = tick_airship,
};
