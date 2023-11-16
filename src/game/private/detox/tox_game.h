// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main header for Detox game project.

#pragma once

#include "../game_private.h"

#define TOX_GAME_MILESTONE     "alive-preview"
#define TOX_GAME_VERSION_MAJOR 0
#define TOX_GAME_VERSION_MINOR 1
#define TOX_GAME_VERSION_PATCH 0

#define TOX_ALIVE_PREVIEW

typedef struct ToxGlobalVars
{
	float timeSpeed;
} ToxGlobalVars;
extern ToxGlobalVars tox_globalVars;

SS_Arl_Camera *tox_get_player_camera( void );
