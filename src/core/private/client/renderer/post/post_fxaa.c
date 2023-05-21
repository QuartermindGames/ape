/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_console.h>

#include "post.h"

/****************************************
 * PRIVATE
 ****************************************/

static OgeMaterial *fxaaMaterial = NULL;

static bool fxaaEnabled = false;

static void RegisterFXAAConsoleVariables( void )
{
	PlRegisterConsoleVariable( "r.fxaa", "Enable FXAA anti-aliasing.", "1", PL_VAR_BOOL, &fxaaEnabled, NULL, true );
}

static bool SetupFXAAEffect( void )
{
	fxaaMaterial = YnCore_Material_Cache( "materials/post/fxaa.mat.n", YN_CORE_CACHE_GROUP_WORLD, false, false );
	if ( fxaaMaterial == NULL )
		return false;

	return true;
}

static void CleanupFXAAEffect( void )
{
	ogeMaterial_Release( fxaaMaterial );
}

static void DrawFXAAEffect( const OgeViewport *viewport )
{
	if ( fxaaEnabled )
		return;

	YnCore_Draw2DQuad( fxaaMaterial, viewport->x, viewport->y, viewport->width, viewport->height );
}

/****************************************
 * PUBLIC
 ****************************************/

const PostProcessEffect *PP_FXAA_GetEffect( void )
{
	static PostProcessEffect renderFXAAPostProcess;
	PL_ZERO_( renderFXAAPostProcess );

	renderFXAAPostProcess.RegisterConsoleVariables = RegisterFXAAConsoleVariables;
	renderFXAAPostProcess.Setup                    = SetupFXAAEffect;
	renderFXAAPostProcess.Cleanup                  = CleanupFXAAEffect;
	renderFXAAPostProcess.Draw                     = DrawFXAAEffect;

	return &renderFXAAPostProcess;
}
