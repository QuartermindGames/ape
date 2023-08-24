// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main header for mag game project.

#pragma once

#include "../game_private.h"

#include "mag_character.h"
#include "mag_world.h"

typedef struct MagGameState {
	MagWorldState worldState;
} MagGameState;
