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
	uint16_t health;
	uint16_t maxHealth;

	GameHealthStatus status;
} GameHealthComponent;
