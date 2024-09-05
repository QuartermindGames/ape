// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Pawns represent anything *living* in the world.
// Author:  Mark E. Sowden

#include "ss1_game.h"
#include "ss1_pawn.h"

static void *create_class( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( SS1Pawn );
}

static void destroy_class( ApeEntity *self )
{
	PL_DELETEN( self->classData );
}

static void spawn_class( ApeEntity *self )
{
}

static AcmBranch *serialize_class( ApeEntity *self )
{
	return nullptr;
}

static void deserialize_class( ApeEntity *self, AcmBranch *root )
{
}

ApeEntityClassDefinition classDefinition = {
        .name                = "SS1Pawn",
        .description         = "Any \"living\" entity in the world.",
        .createFunction      = create_class,
        .destroyFunction     = destroy_class,
        .spawnFunction       = spawn_class,
        .serializeFunction   = serialize_class,
        .deserializeFunction = deserialize_class,
};
