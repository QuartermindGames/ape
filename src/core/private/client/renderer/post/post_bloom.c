/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_console.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_framebuffer.h>

#include "post.h"

/****************************************
 * PRIVATE
 ****************************************/

static ApeShaderProgramIndex *bloomFilterShader;
static ApeShaderProgramIndex *bloomBlurXShader;
static ApeShaderProgramIndex *bloomBlurYShader;

static PLConsoleVariable *bloomEnabled;
static PLConsoleVariable *bloomIntensity;

static PLGFrameBuffer *bloomBuffer;

static PLGTexture *bloomTexture;

static void RegisterBloomConsoleVariables( void )
{
	bloomEnabled   = PlRegisterConsoleVariable( "r.bloom", "Enable/disable bloom effect.", "true", PL_VAR_BOOL, NULL, NULL, true );
	bloomIntensity = PlRegisterConsoleVariable( "r.bloomIntensity", "Set the intensity of the bloom effect.", "0.35", PL_VAR_F32, NULL, NULL, true );
}

static bool SetupBloomEffect( void )
{
	bloomFilterShader = apeGetShaderProgramByName( "post_bloom_filter" );
	if ( bloomFilterShader == NULL )
		return false;
	bloomBlurXShader = apeGetShaderProgramByName( "post_blur_x" );
	if ( bloomBlurXShader == NULL )
		return false;
	bloomBlurYShader = apeGetShaderProgramByName( "post_blur_y" );
	if ( bloomBlurYShader == NULL )
		return false;

	return true;
}

static void CleanupBloomEffect( void )
{
}

static void DrawBloomEffect( const ApeViewport *viewport )
{
	if ( !bloomEnabled->b_value )
		return;

	int bw = viewport->width / 2;
	int bh = viewport->height / 2;

	apeSetupRenderTarget( &bloomBuffer, &bloomTexture, NULL, bw, bh );

	PlgSetCullMode( PLG_CULL_NONE );

	PlgBindFrameBuffer( bloomBuffer, PLG_FRAMEBUFFER_DRAW );

	PlgSetShaderProgram( bloomFilterShader->internalPtr );
	PlgSetShaderUniformValue( bloomFilterShader->internalPtr, "threshold", &bloomIntensity->f_value, false );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, bw, -bh, apeGetPrimaryColourAttachment() );

	PlgSetShaderProgram( bloomBlurXShader->internalPtr );
	PlgSetShaderUniformValue( bloomBlurXShader->internalPtr, "viewportSize", &PLVector2( ( float ) bw, ( float ) bh ), false );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, bw, -bh, bloomTexture );

	PlgSetShaderProgram( bloomBlurYShader->internalPtr );
	PlgSetShaderUniformValue( bloomBlurYShader->internalPtr, "viewportSize", &PLVector2( ( float ) bw, ( float ) bh ), false );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, bw, -bh, bloomTexture );

	PlgBindFrameBuffer( NULL, PLG_FRAMEBUFFER_DEFAULT );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
	PlgSetBlendMode( PLG_BLEND_ONE, PLG_BLEND_ONE );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, viewport->width, -viewport->height, bloomTexture );
}

/****************************************
 * PUBLIC
 ****************************************/

PostProcessEffect *PP_Bloom_GetEffect( void )
{
	static PostProcessEffect renderBloomPostProcess;
	PL_ZERO_( renderBloomPostProcess );

	renderBloomPostProcess.RegisterConsoleVariables = RegisterBloomConsoleVariables;
	renderBloomPostProcess.Setup                    = SetupBloomEffect;
	renderBloomPostProcess.Cleanup                  = CleanupBloomEffect;
	renderBloomPostProcess.Draw                     = DrawBloomEffect;

	return &renderBloomPostProcess;
}
