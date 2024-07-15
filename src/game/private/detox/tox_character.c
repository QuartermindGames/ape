// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Character stats/properties;
// 			a character is basically anything that lives and moves around actively

#include "tox_game.h"
#include "tox_character.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void *create_character_class( ApeEntity *self, AcmBranch *properties );
static void destroy_character_class( ApeEntity *self );
static void spawn_character_class( ApeEntity *self );
static AcmBranch *serialize_character_class( ApeEntity *self );
static void deserialize_character_class( ApeEntity *self, AcmBranch *root );

static const ApeEntityClassDefinition characterClass = {
        .name = "tox_character",
        .createFunction = create_character_class,
        .destroyFunction = destroy_character_class,
        .spawnFunction = spawn_character_class,
        .serializeFunction = serialize_character_class,
        .deserializeFunction = deserialize_character_class,
};

static void *create_character_class( ApeEntity *self, AcmBranch *properties )
{
	return PL_NEW( ToxCharacter );
}

static void destroy_character_class( ApeEntity *self )
{
	PL_DELETEN( self->classData );
}

static void spawn_character_class( ApeEntity *self )
{
}

static void tick_character_class( ApeEntity *self )
{
}

static void draw_character_class( ApeEntity *self )
{
}

static AcmBranch *serialize_character_class( ApeEntity *self )
{
	ToxCharacter *character = self->classData;

	AcmBranch *root = acm_branch_push_back_object( NULL, characterClass.name );
	acm_branch_push_back_int16_array( root, "stats", character->stats, TOX_MAX_CHARACTER_STATS );

	return root;
}

static void deserialize_character_class( ApeEntity *self, AcmBranch *root )
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void tox_character_randomize_stats( ToxCharacter *self )
{
}

int16_t tox_character_xp_to_next( const ToxCharacter *self )
{
	return ( int16_t ) round( 100 * ( self->stats[ TOX_CHARACTER_STAT_LEVEL ] ^ 3 ) );
}

const ApeEntityClassDefinition *tox_characterClass = &characterClass;
