// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Manages randomised ambience.
// Author:  Mark E. Sowden

#include "game_private.h"

static constexpr char GAME_AMBIENCE_MANAGER_CLASS_NAME[] = "ambience_manager";

typedef struct GameAmbienceManagerEntity
{
	ApeStringProperty typeName[ 64 ];
} GameAmbienceManagerEntity;

#define AMBIENCE_MANAGER_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_AMBIENCE_MANAGER_CLASS_NAME, GameAmbienceManagerEntity )

static void *create_ambience_manager( ApeEntity *self, [[maybe_unused]] AcmBranch *properties )
{
	GameAmbienceManagerEntity *manager = QM_OS_MEMORY_NEW( GameAmbienceManagerEntity );
	assert( manager != nullptr );



	return manager;
}

static void destroy_ambience_manager( ApeEntity *self )
{
	GameAmbienceManagerEntity *manager = AMBIENCE_MANAGER_ENTITY( self );
	assert( manager != nullptr );
}

static ApeProperty ambienceManagerProperties[] = {
        APE_PROPERTY_STRING( "Type Name", "Type name, as specified in the ambience script.", GameAmbienceManagerEntity, typeName ),
};

ApeEntityClassDefinition game_ambienceManagerEntityClass_ = {
        .name        = GAME_AMBIENCE_MANAGER_CLASS_NAME,
        .description = "Produces ambient sounds within the environment.",

        .createFunction  = create_ambience_manager,
        .destroyFunction = destroy_ambience_manager,

        .properties    = ambienceManagerProperties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( ambienceManagerProperties ),
};
