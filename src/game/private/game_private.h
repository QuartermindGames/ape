// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_console.h>
#include <plcore/pl_physics.h>
#include <plcore/pl_linkedlist.h>
#include <plcore/pl_hashtable.h>

#include <yin/core.h>
#include <yin/core_entity.h>
#include <yin/core_input.h>
#include <yin/gui_public.h>
#include <yin/node.h>

#include "game/game_interface.h"

#define Game_Print( ... )   PlLogMessage( globalGameLog, __VA_ARGS__ )
#define Game_Warning( ... ) PlLogMessage( globalGameWarningLog, __VA_ARGS__ )
#define Game_Error( ... )   PlLogMessage( globalGameErrorLog, __VA_ARGS__ )
#define Game_Debug( ... )   PlLogMessage( globalGameDebugLog, __VA_ARGS__ )

void gameRegisterStandardEntityComponents( void );
