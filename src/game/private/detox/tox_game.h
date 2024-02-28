// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Main header for Detox game project.

#pragma once

#include "../shared/game_private.h"

#define TOX_GAME_MILESTONE     "proto_a"
#define TOX_GAME_VERSION_MAJOR 0
#define TOX_GAME_VERSION_MINOR 2
#define TOX_GAME_VERSION_PATCH 0

//#define TOX_ALIVE_PREVIEW

typedef struct ToxGlobalVars
{
	float timeSpeed;
} ToxGlobalVars;
extern ToxGlobalVars tox_globalVars;

ApeCamera *tox_get_player_camera( void );
