// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: FXAA
// Author:  Mark E. Sowden

#include "post.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeMaterial *fxaaMaterial = NULL;

static bool fxaaEnabled = false;

static void register_fxaa_console_variables( void )
{
	PlRegisterConsoleVariable( "ape/r/postfx/fxaa", "Enable FXAA anti-aliasing.", "false", PL_VAR_BOOL, &fxaaEnabled, NULL, true );
}

static bool setup_fxaa_effect( void )
{
	fxaaMaterial = ss_arl_material_cache( "materials/post/fxaa.mat.n", APE_CACHE_WORLD, false, false );
	if ( fxaaMaterial == NULL )
		return false;

	return true;
}

static void cleanup_fxaa_effect( void )
{
	ss_arl_material_release( fxaaMaterial );
}

static void draw_fxaa_effect( const ApeViewport *viewport )
{
	if ( !fxaaEnabled )
		return;

	ss_arl_draw_quad( fxaaMaterial, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const SSArlPostProcessEffect *ss_arl_postfx_get_fxaa_( void )
{
	static SSArlPostProcessEffect renderFXAAPostProcess;
	PL_ZERO_( renderFXAAPostProcess );
	renderFXAAPostProcess.RegisterConsoleVariables = register_fxaa_console_variables;
	renderFXAAPostProcess.Setup = setup_fxaa_effect;
	renderFXAAPostProcess.Cleanup = cleanup_fxaa_effect;
	renderFXAAPostProcess.Draw = draw_fxaa_effect;
	return &renderFXAAPostProcess;
}
