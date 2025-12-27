// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef enum GameHealthStatus
{
	GAME_HEALTH_ALIVE,
	GAME_HEALTH_DYING,
	GAME_HEALTH_DEAD,
} GameHealthStatus;

typedef struct GameHealthComponent
{
	int16_t health;
	int16_t maxHealth;

	GameHealthStatus status;
} GameHealthComponent;
