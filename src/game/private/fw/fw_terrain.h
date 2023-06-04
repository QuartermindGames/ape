// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "fw_game.h"

PL_EXTERN_C

void FW_Terrain_Initialize( void );
void FW_Terrain_Shutdown( void );

PLGTexture *FW_Terrain_GetOverview( void );
bool FW_Terrain_Load( const char *path );

float FW_Terrain_GetHeight( float x, float y );
float FW_Terrain_GetMaxHeight( void );
float FW_Terrain_GetMinHeight( void );

void FW_Terrain_Draw( void );

PL_EXTERN_C_END
