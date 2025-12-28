// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: HUD
// Author:  Mark E. Sowden

#include "gateway.h"

static constexpr char HUD_HEALTH_BODY[] = "materials/hud/health_body.mat.n";
static ApeMaterial   *hudHealthBodyMaterial;

static float hudScale = 1.0f;

void gway_hud_initialize_()
{
	PlRegisterConsoleVariable( "gateway_hud.scale",
	                           "Scale of the HUD.",
	                           "1.0",
	                           PL_VAR_F32, &hudScale,
	                           nullptr, true );

	hudHealthBodyMaterial = ape_material_cache( HUD_HEALTH_BODY, APE_CACHE_GROUP_GLOBAL, true );
}

void gway_hud_shutdown_()
{
	ape_material_release( hudHealthBodyMaterial );
}

void gway_hud_draw_( ApeViewport *viewport )
{
	float hudHealthBodyW = 128.0f * hudScale;
	float hudHealthBodyH = 256.0f * hudScale;

	ape_draw_textured_quad( hudHealthBodyMaterial, 16.0f, viewport->height - hudHealthBodyH - 16.0f, hudHealthBodyW, hudHealthBodyH, &QM_MATH_COLOUR4UB( 0, 255, 0, 255 ), 0.0f );
}
