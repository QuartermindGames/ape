// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Character stats/properties.

#pragma once

typedef enum MagCharacterStat
{
	MAG_CHARACTER_STAT_STRENGTH,
	MAG_CHARACTER_STAT_DEXTERITY,
	MAG_CHARACTER_STAT_INTELLIGENCE,
	MAG_CHARACTER_STAT_CHARISMA,
	MAG_CHARACTER_STAT_CONSTITUTION,

	MAG_CHARACTER_STAT_HEALTH,
	MAG_CHARACTER_STAT_MAX_HEALTH,

	MAG_CHARACTER_STAT_STAMINA,
	MAG_CHARACTER_STAT_MAX_STAMINA,

	MAG_CHARACTER_STAT_EXPERIENCE,
	MAG_CHARACTER_STAT_LEVEL,

	MAG_MAX_CHARACTER_STATS
} MagCharacterStat;

typedef int16_t MagCharacterStats[ MAG_MAX_CHARACTER_STATS ];

typedef struct MagCharacter
{
	MagCharacterStats stats;
} MagCharacter;

/**
 * Generates a collection of stats for the character,
 * relative to their current level.
 */
void magRandomizeCharacterStats( MagCharacter *character );

/**
 * Returns the amount of XP required to make
 * it to the next level.
 */
int16_t magNextCharacterLevel( const MagCharacter *character );
