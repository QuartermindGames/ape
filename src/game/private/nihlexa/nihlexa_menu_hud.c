// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: QM1 Hud Implementation
// Author:  Mark E. Sowden

#include "nihlexa_menu.h"

static constexpr char HUD_HEALTH_BODY[] = "materials/hud/health_body.mat.n";
static ApeMaterial   *hudHealthBodyMaterial;

static float hudScale = 1.0f;

void nih_menu_hud_initialize_()
{
	PlRegisterConsoleVariable( "qm1_menu_hud.scale",
	                           "Scale of the HUD.",
	                           "1.0",
	                           PL_VAR_F32, &hudScale,
	                           nullptr, true );

	hudHealthBodyMaterial = ape_material_cache( HUD_HEALTH_BODY, APE_CACHE_GROUP_GLOBAL, true );
}

void nih_menu_hud_shutdown_()
{
	ape_material_release_reference( hudHealthBodyMaterial );
}

void nih_menu_hud_draw_( const ApeViewport *viewport )
{
	static constexpr float BASE_WIDTH  = 128.0f;
	static constexpr float BASE_HEIGHT = 256.0f;

	float hudHealthBodyW = BASE_WIDTH * ( hudScale * shell_get_display_scale() );
	float hudHealthBodyH = BASE_HEIGHT * ( hudScale * shell_get_display_scale() );

	ape_draw_textured_quad( hudHealthBodyMaterial, 16.0f, viewport->height - hudHealthBodyH - 16.0f, hudHealthBodyW, hudHealthBodyH, &QM_MATH_COLOUR4UB( 0, 255, 0, 255 ), 0.0f );
}
