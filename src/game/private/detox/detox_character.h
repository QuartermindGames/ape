// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Character stats/properties.

#pragma once

PL_EXTERN_C

typedef enum ToxCharacterStat {
	TOX_CHARACTER_STAT_STRENGTH,
	TOX_CHARACTER_STAT_DEXTERITY,
	TOX_CHARACTER_STAT_INTELLIGENCE,
	TOX_CHARACTER_STAT_CHARISMA,
	TOX_CHARACTER_STAT_CONSTITUTION,

	TOX_CHARACTER_STAT_HEALTH,
	TOX_CHARACTER_STAT_MAX_HEALTH,

	TOX_CHARACTER_STAT_STAMINA,
	TOX_CHARACTER_STAT_MAX_STAMINA,

	TOX_CHARACTER_STAT_EXPERIENCE,
	TOX_CHARACTER_STAT_LEVEL,

	TOX_MAX_CHARACTER_STATS
} ToxCharacterStat;

typedef int16_t ToxCharacterStats[ TOX_MAX_CHARACTER_STATS ];

typedef struct ToxCharacter {
	ToxCharacterStats stats;
} ToxCharacter;

/**
 * Generates a collection of stats for the character,
 * relative to their current level.
 */
void toxRandomizeCharacterStats( ToxCharacter *character );

/**
 * Returns the amount of XP required to make
 * it to the next level.
 */
int16_t toxNextCharacterLevel( const ToxCharacter *character );

/// Register the character entity class.
const ApeEntityClassDefinition *toxGetCharacterClassTable( void );

PL_EXTERN_C_END
