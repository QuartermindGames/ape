/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_console.h>

#include "post.h"

/****************************************
 * PRIVATE
 ****************************************/

static ApeMaterial *fxaaMaterial = NULL;

static bool fxaaEnabled = false;

static void RegisterFXAAConsoleVariables( void ) {
	PlRegisterConsoleVariable( "r/fxaa", "Enable FXAA anti-aliasing.", "1", PL_VAR_BOOL, &fxaaEnabled, NULL, true );
}

static bool SetupFXAAEffect( void ) {
	fxaaMaterial = apeCacheMaterial( "materials/post/fxaa.mat.n", APE_CACHE_WORLD, false, false );
	if ( fxaaMaterial == NULL )
		return false;

	return true;
}

static void CleanupFXAAEffect( void ) {
	apeReleaseMaterial( fxaaMaterial );
}

static void DrawFXAAEffect( const ApeViewport *viewport ) {
	if ( fxaaEnabled )
		return;

	apeDraw2DQuad( fxaaMaterial, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE );
}

/****************************************
 * PUBLIC
 ****************************************/

const PostProcessEffect *PP_FXAA_GetEffect( void ) {
	static PostProcessEffect renderFXAAPostProcess;
	PL_ZERO_( renderFXAAPostProcess );

	renderFXAAPostProcess.RegisterConsoleVariables = RegisterFXAAConsoleVariables;
	renderFXAAPostProcess.Setup = SetupFXAAEffect;
	renderFXAAPostProcess.Cleanup = CleanupFXAAEffect;
	renderFXAAPostProcess.Draw = DrawFXAAEffect;

	return &renderFXAAPostProcess;
}
