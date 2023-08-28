// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "fw_character.h"

/**
 * This function generates the initial stat set
 * for the given character.
 */
void FW_Character_RollDice( FWCharacterStats *stats ) {
}

/**
 * Returns the amount of XP required to make
 * it to the next level.
 */
uint32_t FW_Character_NextLevel( uint32_t level ) {
	return ( uint32_t ) round( 100 * ( level ^ 3 ) );
}
