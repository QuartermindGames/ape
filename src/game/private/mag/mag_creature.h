// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "../ai/ai_brain.h"


/**
 * These relationships are mostly for the AI,
 * allowing us to determine what characters can
 * attack what.
 */
typedef enum MagCreatureRelationship
{
	MAG_CREATURE_RELATIONSHIP_GOOD,
	MAG_CREATURE_RELATIONSHIP_NEUTRAL,
	MAG_CREATURE_RELATIONSHIP_BAD,
} MagCreatureRelationship;

typedef enum MagCreatureRelationshipGroup
{
	MAG_CREATURE_RELATIONSHIP_GROUP_PLAYER,
	MAG_CREATURE_RELATIONSHIP_GROUP_INNOCENT,
	MAG_CREATURE_RELATIONSHIP_GROUP_CARNIVORE,
	MAG_CREATURE_RELATIONSHIP_GROUP_HERBIVORE,
} MagCreatureRelationshipGroup;

typedef enum MagCreatureEmotion
{
	MAG_CREATURE_EMOTION_ANGER,
} MagCreatureEmotion;

typedef enum MagCreatureSex
{
	MAG_CREATURE_SEX_INTERSEX,
	MAG_CREATURE_SEX_MALE,
	MAG_CREATURE_SEX_FEMALE,

	MAG_CREATURE_MAX_SEXES
} MagCreatureSex;

typedef enum MagCreatureLifePhase
{
	MAG_CREATURE_LIFE_PHASE_BABYHOOD, // immobile and weak
	MAG_CREATURE_LIFE_PHASE_CHILDHOOD,// mobile but weak
	MAG_CREATURE_LIFE_PHASE_ADULTHOOD,// mobile
	MAG_CREATURE_LIFE_PHASE_ELDERLY,  // slower and weaker
	MAG_CREATURE_LIFE_PHASE_EXPIRED,  // dead

	MAG_CREATURE_MAX_LIFE_PHASES
} MagCreatureLifePhase;

#define MAG_CREATURE_MAGIC PL_MAGIC_TO_NUM( 'M', 'C', 'R', 'T' )

typedef struct MagCreature
{
	unsigned int magic;

	MagCreatureLifePhase phase;
	unsigned int age;
	unsigned int maxAge;
	unsigned int generation;

	MagCreatureSex sex;
	bool isPregnant;
	unsigned int timePregnant;

	int stamina;
	int maxStamina;

	int hunger;
	int thirst;

	unsigned int stepTime;

	unsigned int experience;
	unsigned int maxExperience;
	unsigned int level;

	AIBrain brain;
} MagCreature;
