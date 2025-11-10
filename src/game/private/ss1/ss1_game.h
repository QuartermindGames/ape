// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../shared/game_private.h"
#include "../shared/game_server.h"
#include "../shared/game_client.h"
#include "../shared/game_world_simulation.h"

static constexpr char QM1_GAME_TITLE[] = "Nihlexa";

#define SS1_GAME_MILESTONE     "ss1_proto_1"
#define SS1_GAME_VERSION_MAJOR 0
#define SS1_GAME_VERSION_MINOR 0
#define SS1_GAME_VERSION_PATCH 0

#define SS1_GAME_PROTOCOL_VERSION ( GAME_NET_PROTOCOL_VERSION + 1 )

#define SS1_MAX_TEAM_NAME   32
#define SS1_MAX_PLAYER_NAME 32

static constexpr unsigned int QM1_GAME_MAX_TEAMS      = 4;
static constexpr unsigned int QM1_GAME_MAX_PLAYERS    = 4;
static constexpr unsigned int QM1_GAME_MAX_CHARACTERS = 8;

#include "qm1_character.h"

typedef struct Qm1Team
{
	Qm1Character characters[ QM1_GAME_MAX_CHARACTERS ];
	unsigned int numCharacters;
} Qm1Team;

typedef enum SS1ResourceType : uint8_t
{
	SS1_RESOURCE_TYPE_MANA,
	SS1_RESOURCE_TYPE_GOLD,

	SS1_MAX_RESOURCE_TYPES
} SS1ResourceType;

#define SS1_DEFAULT_SUN_POSITION QM_MATH_VECTOR3F( -2.0f, -2.0f, 0.0f )
#define SS1_DEFAULT_SUN_COLOUR   QM_MATH_COLOUR4F( 1.0f, 1.0f, 1.0f, 1.85f )
#define SS1_DEFAULT_CLEAR_COLOUR QM_MATH_COLOUR4F( 0.1f, 0.5f, 1.0f, 1.0f )

#define SS1_DEFAULT_MOON_COLOUR QM_MATH_COLOUR4F( 0.2f, 0.2f, 0.5f, 0.0f )

typedef enum GameCameraState
{
	GAME_CAMERA_STATE_FREE,        // free camera
	GAME_CAMERA_STATE_FIXED,       // game has control over it
	GAME_CAMERA_STATE_FIRST_PERSON,// first-person w/ input
	GAME_CAMERA_STATE_THIRD_PERSON,// third-person w/ input

	GAME_CAMERA_STATE_MAX,
} GameCameraState;

typedef enum Qm1RoundStatus
{
	QM1_ROUND_STATUS_INTRO,
	QM1_ROUND_STATUS_SELECT,
	QM1_ROUND_STATUS_SELECTED,
	QM1_ROUND_STATUS_PLAYING,
	QM1_ROUND_STATUS_END,
} Qm1RoundStatus;

typedef struct SS1GameState
{
	GameWorldSimulation simulation;

	GamePlayer players[ QM1_GAME_MAX_PLAYERS ];

	ApeCamera      *camera;// our eyes
	QmMathVector3f  oldCameraPosition;
	GameCameraState oldCameraState, cameraState;

	Qm1RoundStatus roundStatus;

	ApeWorld *world;// world container

	ApeLight *moonLight;
	float     moonBrightness;

	ApeLight      *sunLight;
	QmMathVector2f sunAngles;
	float          sunBrightness;

	bool       isFirstLaunch;
	AcmBranch *config;
} SS1GameState;
extern SS1GameState ss1_gameState;
