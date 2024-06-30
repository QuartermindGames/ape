// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../shared/game_private.h"
#include "../shared/game_world_simulation.h"

#define SS1_GAME_MILESTONE     "ss1_proto_1"
#define SS1_GAME_VERSION_MAJOR 0
#define SS1_GAME_VERSION_MINOR 0
#define SS1_GAME_VERSION_PATCH 0

#define SS1_GAME_PROTOCOL_VERSION ( GAME_NET_PROTOCOL_VERSION + 1 )

#define SS1_MAX_TEAM_NAME   32
#define SS1_MAX_PLAYER_NAME 32

#define SS1_MAX_TEAMS   2
#define SS1_MAX_PLAYERS 64

typedef enum SS1ProfessionType : uint8_t
{
	SS1_PROFESSION_SHAMAN,   // medic
	SS1_PROFESSION_MACHINIST,// engineer
	SS1_PROFESSION_TRICKSTER,// spy
	SS1_PROFESSION_POUNDER,  // soldier

	SS1_MAX_PROFESSIONS
} SS1ProfessionType;

typedef struct SS1Profession
{
	const char *name;
	const char *description;

	float maxForwardSpeed;
	float maxStrafeSpeed;
} SS1Profession;
extern const SS1Profession ss1_professions[ SS1_MAX_PROFESSIONS ];

typedef struct SS1Team
{
	char name[ SS1_MAX_TEAM_NAME ];
} SS1Team;

typedef struct SS1Player
{
	char name[ SS1_MAX_PLAYER_NAME ];

	SS1Team *team;
} SS1Player;

typedef struct SS1GameState
{
	WorldSimulation simulation;

	SS1Team teams[ SS1_MAX_TEAMS ];
	SS1Player players[ SS1_MAX_PLAYERS ];

	ApeCamera *camera;// our eyes
	ApeWorld *world;  // world container
	ApeRoom *room;    // everything in the scene should be tied to this!
	ApeBrush *terrain;// proc terrain brush

	bool isFirstLaunch;
	NdBranch *config;
} SS1GameState;
extern SS1GameState ss1_gameState;
