// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main header for Detox game project.

#pragma once

#include "../game_private.h"

#define DETOX_GAME_MILESTONE     "alive-preview"
#define DETOX_GAME_VERSION_MAJOR 0
#define DETOX_GAME_VERSION_MINOR 1
#define DETOX_GAME_VERSION_PATCH 0

typedef struct ToxGlobalVars
{
	float sunPitch;
	float sunYaw;
} ToxGlobalVars;
extern ToxGlobalVars tox_globalVars;
