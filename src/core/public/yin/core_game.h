// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

typedef struct GameState {
	int mode, oldMode;
} GameState;
extern GameState acl_gameState_;

extern const struct GameModeInterface *game_modeInterface;

void acl_initialize_game_( void );
void acl_shutdown_game_( void );
void apeTickGame( void );
void apeDisconnectGame( void );
void apeSpawnWorld( const char *worldPath );

PL_EXTERN_C_END
