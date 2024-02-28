// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "fw_game.h"

PL_EXTERN_C

void fw_terrain_initialize( void );
void fw_terrain_shutdown( void );

PLGTexture *fw_terrain_get_overview( void );
bool fw_terrain_load( const char *path );

float fw_terrain_get_height( float x, float y );
float fw_terrain_get_max_height( void );
float fw_terrain_get_min_height( void );

void FW_Terrain_Draw( void );

PL_EXTERN_C_END
