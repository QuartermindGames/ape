// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../shared/game_private.h"
#include "../shared/game_server.h"
#include "../shared/game_client.h"
#include "../shared/game_world_simulation.h"

#define SS1_GAME_TITLE "embrace INC."

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

	unsigned int maxHealth;
} SS1Profession;
extern const SS1Profession ss1_professions[ SS1_MAX_PROFESSIONS ];

typedef enum SS1ResourceType : uint8_t
{
	SS1_RESOURCE_TYPE_MANA,
	SS1_RESOURCE_TYPE_GOLD,

	SS1_MAX_RESOURCE_TYPES
} SS1ResourceType;

#define SS1_DEFAULT_SUN_POSITION PL_VECTOR3( -2.0f, -2.0f, 0.0f )
#define SS1_DEFAULT_SUN_COLOUR   PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.85f )
#define SS1_DEFAULT_CLEAR_COLOUR PL_COLOURF32( 0.1f, 0.5f, 1.0f, 1.0f )

#define SS1_DEFAULT_MOON_COLOUR PL_COLOURF32( 0.2f, 0.2f, 0.5f, 0.0f )

typedef enum SS1CameraState
{
	SS1_CAMERA_STATE_FREE,
	SS1_CAMERA_STATE_FIRST_PERSON,
	SS1_CAMERA_STATE_THIRD_PERSON,
} SS1CameraState;

typedef struct SS1GameState
{
	GameWorldSimulation simulation;

	GamePlayer players[ SS1_MAX_PLAYERS ];

	ApeCamera     *camera;// our eyes
	PLVector3      oldCameraPosition;
	SS1CameraState oldCameraState, cameraState;

	ApeWorld *world;// world container

	ApeLight *moonLight;
	float     moonBrightness;

	ApeLight *sunLight;
	PLVector2 sunAngles;
	float     sunBrightness;

	bool       isFirstLaunch;
	AcmBranch *config;
} SS1GameState;
extern SS1GameState ss1_gameState;
