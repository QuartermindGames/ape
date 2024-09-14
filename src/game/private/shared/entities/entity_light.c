// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: <purpose>
// Author:  <name>

#include "../game_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct LightEntity
{
	ApeLight *light;
} LightEntity;
#define LIGHT( SELF ) APE_ENT_CLASS( ( SELF ), LightEntity )

static void *create_light( PL_UNUSED ApeEntity *self, PL_UNUSED AcmBranch *properties )
{
	return PL_NEW( LightEntity );
}

static void spawn_light( ApeEntity *self )
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeEntityClassDefinition game_entityLightClass = {
        .name = "light",
        .description = "Light",
        .createFunction = create_light,
        .spawnFunction = spawn_light,
};
