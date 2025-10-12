// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef enum Qm1CharacterProfessionType : uint8_t
{
	QM1_CHARACTER_PROFESSION_SHAMAN,   // medic
	QM1_CHARACTER_PROFESSION_MACHINIST,// engineer
	QM1_CHARACTER_PROFESSION_TRICKSTER,// spy
	QM1_CHARACTER_PROFESSION_POUNDER,  // soldier

	QM1_CHARACTER_MAX_PROFESSIONS
} Qm1CharacterProfessionType;

typedef struct Qm1CharacterProfession
{
	const char *name;
	const char *description;

	float maxForwardSpeed;
	float maxStrafeSpeed;

	unsigned int maxHealth;
} Qm1CharacterProfession;
extern const Qm1CharacterProfession qm1_professions_[ QM1_CHARACTER_MAX_PROFESSIONS ];

static constexpr unsigned int QM1_CHARACTER_MAX_NAME_ = 64;

/**
 * This is the struct we'll be using to store all the fancy character stats.
 */
typedef struct Qm1Character
{
	char name[ QM1_CHARACTER_MAX_NAME_ ];

	Qm1CharacterProfession profession;
} Qm1Character;
