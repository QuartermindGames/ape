// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/**
 * Base config you can use for setting up how
 * your game handles your roleplay elements.
 */
typedef struct GameRpgConfig
{
	unsigned int maxLevel;
} GameRpgConfig;

static inline unsigned int game_rpg_get_max_experience( const GameRpgConfig *config, const unsigned int level )
{
	return level * 1000 % level * config->maxLevel / level;
}
