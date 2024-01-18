// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "fw_game.h"

typedef enum FWCharacterDepartment
{
	FW_CHARACTER_DEPARTMENT_MARINES,
	FW_CHARACTER_DEPARTMENT_INTELLIGENCE,// special ops
	FW_CHARACTER_DEPARTMENT_MEDICAL,     // healing items/abilities
	FW_CHARACTER_DEPARTMENT_ENGINEERING, // vehicle/building repair
	FW_CHARACTER_DEPARTMENT_CHEMICALS,   // chemical-based weaponry

	FW_MAX_CHARACTER_DEPARTMENTS
} FWCharacterDepartment;

// certain classes provide buffs
typedef struct FWCharacterDepartmentStats
{
	uint16_t health;
	uint16_t stamina;
	uint16_t strength;
	uint16_t speed;
} FWCharacterDepartmentStats;

typedef struct FWCharacterStats
{
	uint16_t experience, maxExperience;
	uint16_t rank;

	uint16_t strength;
	uint16_t speed;
} FWCharacterStats;

typedef struct FWCharacterMovementComponent
{
	float velocity;
} FWCharacterMovementComponent;

typedef struct FWCharacterComponent
{
	FWCharacterDepartment department;
	FWCharacterStats stats;

	int health, maxHealth;
	int stamina, maxStamina;

	PLLinkedList *buildings;

	SS_Acl_EntityComponent *transformComponent;
	SS_Acl_EntityComponent *meshComponent;
	SS_Acl_EntityComponent *movementComponent;
} FWCharacterComponent;

/**
 * This function generates the initial stat set
 * for the given character.
 */
void FW_Character_RollDice( FWCharacterStats *stats );

/**
 * Returns the amount of XP required to make
 * it to the next level.
 */
uint32_t FW_Character_NextLevel( uint32_t level );
