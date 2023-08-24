// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

typedef struct GameState {
	int mode, oldMode;
} GameState;
extern GameState oge_gameState_;

extern const struct GameModeInterface *game_modeInterface;

void apeInitializeGame( void );
void apeShutdownGame( void );
void apeTickGame( void );
void apeDisconnectGame( void );
void apeSpawnWorld( const char *worldPath );

PL_EXTERN_C_END
