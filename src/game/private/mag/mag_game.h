// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main header for Magonia game project.

#pragma once

#include "../game_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// MAG Tile Editor

typedef enum MagTileEditorStatus
{
	MAG_TILE_EDITOR_STATUS_CLOSED,
	MAG_TILE_EDITOR_STATUS_OPEN,
	MAG_TILE_EDITOR_STATUS_CLOSING,
} MagTileEditorStatus;

void mag_tile_editor_initialize( void );

MagTileEditorStatus mag_tile_editor_get_status( void );

void mag_tile_editor_draw( SSArlViewport *viewport );
void mag_tile_editor_draw_ui( SSArlViewport *viewport );
void mag_tile_editor_tick( void );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
