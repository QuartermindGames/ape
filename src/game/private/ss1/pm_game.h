// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../shared/game_private.h"
#include "../shared/game_world_simulation.h"

#define PM_GAME_MILESTONE     "pm_proto_1"
#define PM_GAME_VERSION_MAJOR 0
#define PM_GAME_VERSION_MINOR 0
#define PM_GAME_VERSION_PATCH 0

#define PM_MAX_TEAMS        4
#define PM_MAX_TEAM_MEMBERS 64
#define PM_MAX_TEAM_PLAYERS PM_MAX_TEAM_MEMBERS

#define PM_MAX_TEAM_NAME   32
#define PM_MAX_PLAYER_NAME 32

#define PM_MAX_PLAYERS 16

typedef struct PMTeam
{
	char name[ PM_MAX_TEAM_NAME ];
} PMTeam;

typedef struct PMPlayer
{
	char name[ PM_MAX_PLAYER_NAME ];

	PMTeam *team;
} PMPlayer;

typedef struct PMGameState
{
	WorldSimulation simulation;

	PMTeam teams[ PM_MAX_TEAMS ];
	PMPlayer players[ PM_MAX_PLAYERS ];

	ApeCamera *camera;
	ApeWorld *world;
	ApeBrush *terrain;

	bool isFirstLaunch;
	NdBranch *config;
} PMGameState;
extern PMGameState pm_gameState;
