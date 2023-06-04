// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

typedef struct GameState
{
	int mode, oldMode;
} GameState;
extern GameState oge_gameState_;

void apeInitializeGame( void );
void apeShutdownGame( void );
void apeTickGame( void );
void apeDisconnectGame( void );

PL_EXTERN_C_END
