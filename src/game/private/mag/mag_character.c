// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Character stats/properties.

#include "mag_game.h"
#include "mag_character.h"

void magRandomizeCharacterStats( MagCharacter *character ) {
}

int16_t magNextCharacterLevel( const MagCharacter *character ) {
	return ( int16_t ) round( 100 * ( character->stats[ MAG_CHARACTER_STAT_LEVEL ] ^ 3 ) );
}
