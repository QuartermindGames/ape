// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

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
	MAG_CREATURE_EMOTION_RELAXED,
	MAG_CREATURE_EMOTION_ANGRY,
	MAG_CREATURE_EMOTION_EXCITED,
	MAG_CREATURE_EMOTION_DEPRESSED,
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

typedef enum MagCreatureIntelligence
{
	MAG_CREATURE_INTELLIGENCE_LOW,     // like a dog/cat
	MAG_CREATURE_INTELLIGENCE_MEDIUM,  // primitive human or ape
	MAG_CREATURE_INTELLIGENCE_ADVANCED,// human being
} MagCreatureIntelligence;

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
