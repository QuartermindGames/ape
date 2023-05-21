// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

typedef struct GameState
{
	int mode, oldMode;
} GameState;
extern GameState gameState;

void ogeInitializeGame( void );
void ogeShutdownGame( void );
void Game_Tick( void );
void Game_Disconnect( void );

PL_EXTERN_C_END
