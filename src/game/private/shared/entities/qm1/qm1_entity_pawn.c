// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Pawns represent anything *living* in the world.
// Author:  Mark E. Sowden

#include "ss1/ss1_game.h"

#include "qm1_entity_pawn.h"

static void *create_class( ApeEntity *self, AcmBranch *properties )
{
	return QM_OS_MEMORY_NEW( SS1Pawn );
}

static void destroy_class( ApeEntity *self )
{
	qm_os_memory_free( self->classData );
	self->classData = nullptr;
}

static void spawn_class( ApeEntity *self )
{
}

ApeEntityClassDefinition ss1_pawnEntityClass = {
        .name            = "ss1_pawn",
        .description     = "Any \"living\" entity in the world.",
        .createFunction  = create_class,
        .destroyFunction = destroy_class,
        .spawnFunction   = spawn_class,
};
