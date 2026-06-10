// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "rpg.h"

typedef struct GameRpgBaseComponent
{
	unsigned int level;

	unsigned int experience;
	unsigned int maxExperience;

	uint8_t stats[ GAME_RPG_STAT_TYPE_MAX ];
} GameRpgBaseComponent;
