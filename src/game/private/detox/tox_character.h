// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Character stats/properties.

#pragma once

PL_EXTERN_C

/**
 * These relationships are mostly for the AI,
 * allowing us to determine what characters can
 * attack what.
 */
typedef enum ToxCharacterRelationship
{
	TOX_CHARACTER_RELATIONSHIP_GOOD,
	TOX_CHARACTER_RELATIONSHIP_NEUTRAL,
	TOX_CHARACTER_RELATIONSHIP_BAD,
} ToxCharacterRelationship;

typedef enum ToxCharacterRelationshipGroup
{
	TOX_CHARACTER_RELATIONSHIP_GROUP_PLAYER,
	TOX_CHARACTER_RELATIONSHIP_GROUP_EVIL,
	TOX_CHARACTER_RELATIONSHIP_GROUP_CARNIVORE,
	TOX_CHARACTER_RELATIONSHIP_GROUP_HERBIVORE,
} ToxCharacterRelationshipGroup;

typedef enum ToxCharacterAttribute
{
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
} ToxCharacterAttribute;

typedef int16_t ToxCharacterStats[ TOX_MAX_CHARACTER_STATS ];

typedef struct ToxCharacter
{
	ToxCharacterStats stats;
} ToxCharacter;

/**
 * Generates a collection of stats for the character,
 * relative to their current level.
 */
void tox_character_randomize_stats( ToxCharacter *character );

/**
 * Returns the amount of XP required to make
 * it to the next level.
 */
int16_t tox_character_xp_to_next( const ToxCharacter *self );

PL_EXTERN_C_END
