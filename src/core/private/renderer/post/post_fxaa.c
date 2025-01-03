// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: FXAA
// Author:  Mark E. Sowden

#include "post.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeMaterial *fxaaMaterial = NULL;

static bool fxaaEnabled = false;

static void register_fxaa_console_variables( void )
{
	PlRegisterConsoleVariable( "postfx_fxaa", "Enable FXAA anti-aliasing.", "false", PL_VAR_BOOL, &fxaaEnabled, NULL, true );
}

static bool setup_fxaa_effect( void )
{
	fxaaMaterial = ape_material_cache( "materials/post/fxaa.mat.n", APE_CACHE_GROUP_WORLD, false );
	if ( fxaaMaterial == NULL )
	{
		return false;
	}

	return true;
}

static void cleanup_fxaa_effect( void )
{
	ape_material_release( fxaaMaterial );
}

static void draw_fxaa_effect( const ApeViewport *viewport )
{
	if ( !fxaaEnabled )
	{
		return;
	}

	ape_draw_textured_quad( fxaaMaterial, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const ApePostProcessEffect *ape_postfx_get_fxaa_( void )
{
	static ApePostProcessEffect renderFXAAPostProcess;
	PL_ZERO_( renderFXAAPostProcess );
	renderFXAAPostProcess.registerConsoleVariables = register_fxaa_console_variables;
	renderFXAAPostProcess.setup = setup_fxaa_effect;
	renderFXAAPostProcess.cleanup = cleanup_fxaa_effect;
	renderFXAAPostProcess.draw = draw_fxaa_effect;
	return &renderFXAAPostProcess;
}
