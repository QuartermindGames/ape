// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

typedef struct GameState {
	int mode, oldMode;
} GameState;
extern GameState acl_gameState_;

extern const struct GameModeInterface *game_modeInterface;

PL_EXTERN_C_END
