// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

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
