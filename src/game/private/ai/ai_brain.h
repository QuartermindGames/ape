// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef enum AIBrainMotorAction
{
	AI_BRAIN_MOTOR_ACTION_USE,
	AI_BRAIN_MOTOR_ACTION_DRINK,
	AI_BRAIN_MOTOR_ACTION_EAT,
	AI_BRAIN_MOTOR_ACTION_TALK,
	AI_BRAIN_MOTOR_ACTION_ATTACK,
	AI_BRAIN_MOTOR_ACTION_APPROACH,
	AI_BRAIN_MOTOR_ACTION_RETREAT,
} AIBrainMotorAction;

typedef struct AIBrainNeed
{
	const char *description;
	double      value;
} AIBrainNeed;

typedef enum AIBrainDisposition
{
	AI_BRAIN_DISPOSITION_NEUTRAL,
	AI_BRAIN_DISPOSITION_POSITIVE,
	AI_BRAIN_DISPOSITION_NEGATIVE,
} AIBrainDisposition;

typedef struct AIBrainMemory
{
} AIBrainMemory;

typedef enum AIBrainMood
{
	AI_BRAIN_MOOD_SAD,
	AI_BRAIN_MOOD_NEUTRAL,
	AI_BRAIN_MOOD_HAPPY,
	AI_BRAIN_MOOD_ANGRY,
	AI_BRAIN_MOOD_EXCITED,
	AI_BRAIN_MOOD_DEPRESSED,

	AI_MAX_MOODS
} AIBrainMood;

#define AI_BRAIN_MAX_DIRECTIVES     64
#define AI_BRAIN_MAX_SUB_DIRECTIVES 32

typedef struct AIBrainDirective
{
	AIBrainMotorAction       type;
	double                   weight;
	bool                     completed;
	const char              *description;
	struct AIBrainDirective *subDirectives;
	unsigned int             numSubDirectives;
} AIBrainDirective;

typedef struct AIBrain
{
	bool active;

	AIBrainMood mood;

	AIBrainDirective directives[ AI_BRAIN_MAX_DIRECTIVES ];
	unsigned int     numDirectives;
} AIBrain;

void game_ai_brain_tick_( AIBrain *brain, double delta );
