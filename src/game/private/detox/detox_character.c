// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Character stats/properties.

#include "detox_game.h"
#include "detox_character.h"

/****************************************
 * PRIVATE
 ****************************************/

static const char *className = "tox/character";

static void *CreateCharacterClass( ApeEntity *self, NdBranch *properties ) {
	return PL_NEW( ToxCharacter );
}

static void DestroyCharacterClass( ApeEntity *self ) {
	PL_DELETEN( self->classData );
}

static NdBranch *SerializeCharacterClass( ApeEntity *self ) {
	ToxCharacter *character = self->classData;

	NdBranch *root = ndPushBackObject( NULL, className );
	ndPushBackI16Array( root, "stats", character->stats, TOX_MAX_CHARACTER_STATS );

	return root;
}

static void DeserializeCharacterClass( ApeEntity *self, NdBranch *root ) {
}

/****************************************
 * PUBLIC
 ****************************************/

void toxRandomizeCharacterStats( ToxCharacter *character ) {
}

int16_t toxNextCharacterLevel( const ToxCharacter *character ) {
	return ( int16_t ) round( 100 * ( character->stats[ TOX_CHARACTER_STAT_LEVEL ] ^ 3 ) );
}

const ApeEntityClassDefinition *toxGetCharacterClassTable( void ) {
	static ApeEntityClassDefinition table;
	PL_ZERO_( table );
	table.name = className;
	table.Create = CreateCharacterClass;
	table.Destroy = DestroyCharacterClass;
	table.Serialize = SerializeCharacterClass;
	table.Deserialize = DeserializeCharacterClass;
	return &table;
}
