// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Main header for Detox game project.

#pragma once

#include "../shared/game_private.h"
#include "../shared/game_server.h"
#include "../shared/game_client.h"

#define TOX_GAME_MILESTONE     "proto_a"
#define TOX_GAME_VERSION_MAJOR 0
#define TOX_GAME_VERSION_MINOR 2
#define TOX_GAME_VERSION_PATCH 0

#define TOX_GAME_PROTOCOL_VERSION ( GAME_NET_PROTOCOL_VERSION + 1 )

//#define TOX_ALIVE_PREVIEW

typedef enum ToxCameraState
{
	TOX_CAMERA_FLY,
	TOX_CAMERA_ORBIT,
} ToxCameraState;

typedef struct ToxGlobalVars
{
	ToxCameraState cameraState;

	float timeSpeed;
} ToxGlobalVars;
extern ToxGlobalVars tox_globalVars;

ApeCamera *tox_get_player_camera( void );
