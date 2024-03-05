// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Character stats/properties;
// 			a character is basically anything that lives and moves around actively

#include "tox_game.h"
#include "tox_character.h"

/****************************************
 * PRIVATE
 ****************************************/

static const char *className = "tox_character";

static void *CreateCharacterClass( ApeEntity *self, NdBranch *properties )
{
	return PL_NEW( ToxCharacter );
}

static void DestroyCharacterClass( ApeEntity *self )
{
	PL_DELETEN( self->classData );
}

static NdBranch *SerializeCharacterClass( ApeEntity *self )
{
	ToxCharacter *character = self->classData;

	NdBranch *root = nd_branch_push_back_object( NULL, className );
	nd_branch_push_back_int16_array( root, "stats", character->stats, TOX_MAX_CHARACTER_STATS );

	return root;
}

static void DeserializeCharacterClass( ApeEntity *self, NdBranch *root )
{
}

/****************************************
 * PUBLIC
 ****************************************/

void tox_character_randomize_stats( ToxCharacter *character )
{
}

int16_t tox_character_xp_to_next( const ToxCharacter *character )
{
	return ( int16_t ) round( 100 * ( character->stats[ TOX_CHARACTER_STAT_LEVEL ] ^ 3 ) );
}

const ApeEntityClassDefinition *tox_character_get_class_table( void )
{
	static ApeEntityClassDefinition table;
	PL_ZERO_( table );
	table.name = className;
	table.createFunction = CreateCharacterClass;
	table.destroyFunction = DestroyCharacterClass;
	table.serializeFunction = SerializeCharacterClass;
	table.deserializeFunction = DeserializeCharacterClass;
	return &table;
}
