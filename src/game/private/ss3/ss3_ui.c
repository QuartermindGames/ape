// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: For handling general UI routines.
// Author:  Mark E. Sowden

#include "ss3_game.h"

static ApeMaterial *hudBodyScan;

void ss3_ui_initialize( void )
{
	hudBodyScan = ape_material_cache( "materials/hud/hud_bodyscan.mat.n", APE_CACHE_GROUP_WORLD, true, false );
}

void ss3_ui_shutdown( void )
{
	ape_material_release( hudBodyScan );
	hudBodyScan = NULL;
}

void ss3_ui_tick( void )
{
}

bool ss3_ui_draw( ApeViewport *viewport )
{
	//ape_draw_textured_quad( hudBodyScan, 0.0f, 0.0f, 100.0f, 100.0f, &PL_COLOURU8( 0, 255, 0, 255 ) );
	return true;
}
