// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef enum GameRpgStatType : uint8_t
{
	GAME_RPG_STAT_TYPE_STR,// strength
	GAME_RPG_STAT_TYPE_DEX,// dexterity
	GAME_RPG_STAT_TYPE_END,// edurance; how much you can carry etc.
	GAME_RPG_STAT_TYPE_INT,// intelligence
	GAME_RPG_STAT_TYPE_AGI,// agility

	GAME_RPG_STAT_TYPE_MAX
} GameRpgStatType;

typedef struct GameRpgSkill
{

} GameRpgSkill;

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
