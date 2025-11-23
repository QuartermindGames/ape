// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Basic RPG component for leveling, experience and stats.
// Author:  Mark E. Sowden

#include "game_private.h"

#include "rpg_component.h"

static void *create_base_component()
{
	return QM_OS_MEMORY_NEW( GameRpgBaseComponent );
}

static void destroy_base_component( void *data )
{
	qm_os_memory_free( data );
}

ApeEntityComponentDefinition game_rpgBaseComponent_ = {
        .name            = "rpg_base",
        .createFunction  = create_base_component,
        .destroyFunction = destroy_base_component,
};
