// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Pawns represent anything *living* in the world.

#pragma once

/**
 * These relationships are mostly for the AI,
 * allowing us to determine what characters can
 * attack what.
 */
typedef enum SS1PawnRelationship
{
	SS1_PAWN_RELATIONSHIP_GOOD,
	SS1_PAWN_RELATIONSHIP_NEUTRAL,
	SS1_PAWN_RELATIONSHIP_BAD,
} SS1PawnRelationship;

typedef enum SS1PawnRelationshipGroup
{
	SS1_PAWN_RELATIONSHIP_GROUP_PLAYER,
	SS1_PAWN_RELATIONSHIP_GROUP_EVIL,
	SS1_PAWN_RELATIONSHIP_GROUP_CARNIVORE,
	SS1_PAWN_RELATIONSHIP_GROUP_HERBIVORE,
} SS1PawnRelationshipGroup;

typedef enum SS1PawnAttribute
{
	SS1_PAWN_STAT_HEALTH,
	SS1_PAWN_STAT_MAX_HEALTH,

	SS1_PAWN_STAT_STAMINA,
	SS1_PAWN_STAT_MAX_STAMINA,

	SS1_MAX_PAWN_STATS
} SS1PawnAttribute;

typedef int16_t SS1PawnStats[ SS1_MAX_PAWN_STATS ];

typedef struct SS1Pawn
{
	SS1PawnStats             stats;
	SS1PawnRelationshipGroup relationshipGroup;
	unsigned int             team;

	struct ApeModelNode *model;
} SS1Pawn;
