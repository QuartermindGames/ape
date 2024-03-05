// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: BLOOOOOM
// Author:  Mark E. Sowden

#include "post.h"

#include "../renderer_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static SS_Arl_ShaderProgramIndex *bloomFilterShader;
static SS_Arl_ShaderProgramIndex *bloomBlurXShader;
static SS_Arl_ShaderProgramIndex *bloomBlurYShader;

static ApeRenderTarget *bloomRenderTarget;

static bool bloomEnabled;
static float bloomIntensity;

static void register_bloom_console_variables( void )
{
	PlRegisterConsoleVariable( "postfx_bloom", "Enable/disable bloom effect.", "true", PL_VAR_BOOL, &bloomEnabled, NULL, true );
	PlRegisterConsoleVariable( "postfx_bloom_intensity", "Set the intensity of the bloom effect.", "0.75", PL_VAR_F32, &bloomIntensity, NULL, true );
}

static bool setup_bloom_effect( void )
{
	bloomFilterShader = ape_shader_get_by_name( "post_bloom_filter" );
	if ( bloomFilterShader == NULL )
		return false;
	bloomBlurXShader = ape_shader_get_by_name( "post_blur_x" );
	if ( bloomBlurXShader == NULL )
		return false;
	bloomBlurYShader = ape_shader_get_by_name( "post_blur_y" );
	if ( bloomBlurYShader == NULL )
		return false;

	bloomRenderTarget = ape_render_target_create( "post_bloom", 800, 600, PLG_BUFFER_COLOUR, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR );
	if ( bloomRenderTarget == NULL )
	{
		PRINT_WARNING( "Failed to create render target for bloom effect!\n" );
		return false;
	}

	return true;
}

static void cleanup_bloom_effect( void )
{
	ape_render_target_release( bloomRenderTarget );
}

static void draw_bloom_effect( const ApeViewport *viewport )
{
	if ( !bloomEnabled || viewport->renderTarget == NULL )
		return;

	PLGTexture *baseTexture = ape_render_target_get_texture( viewport->renderTarget );
	if ( baseTexture == NULL )
		return;

	int bw = viewport->width / 2;
	int bh = viewport->height / 2;

	ape_render_target_set_size( bloomRenderTarget, bw, bh );
	PLGTexture *bloomRenderTargetTexture = ape_render_target_get_texture( bloomRenderTarget );
	if ( bloomRenderTargetTexture == NULL )
		return;

	ape_render_target_bind( bloomRenderTarget, PLG_FRAMEBUFFER_DRAW );
	PlgClearBuffers( PLG_BUFFER_COLOUR );

	PlgSetCullMode( PLG_CULL_NONE );

	PlgSetShaderProgram( bloomFilterShader->internal );
	PlgSetShaderUniformValue( bloomFilterShader->internal, "threshold", &bloomIntensity, false );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, bw, -bh, baseTexture );

	ape_render_target_bind( bloomRenderTarget, PLG_FRAMEBUFFER_DEFAULT );

	PlgSetShaderProgram( bloomBlurXShader->internal );
	PlgSetShaderUniformValue( bloomBlurXShader->internal, "viewportSize", &PLVector2( ( float ) bw, ( float ) bh ), false );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, bw, -bh, bloomRenderTargetTexture );

	PlgSetShaderProgram( bloomBlurYShader->internal );
	PlgSetShaderUniformValue( bloomBlurYShader->internal, "viewportSize", &PLVector2( ( float ) bw, ( float ) bh ), false );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, bw, -bh, bloomRenderTargetTexture );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );

	//TODO: this last step is botched, urgh...

	ape_render_target_bind( ape_postfx_get_render_target(), PLG_FRAMEBUFFER_DRAW );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, viewport->width, -viewport->height, baseTexture );
	PlgSetBlendMode( PLG_BLEND_ADDITIVE );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, viewport->width, -viewport->height, bloomRenderTargetTexture );
	PlgSetBlendMode( PLG_BLEND_DISABLE );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const SSArlPostProcessEffect *ss_arl_postfx_get_bloom_( void )
{
	static SSArlPostProcessEffect renderBloomPostProcess;
	PL_ZERO_( renderBloomPostProcess );
	renderBloomPostProcess.RegisterConsoleVariables = register_bloom_console_variables;
	renderBloomPostProcess.Setup = setup_bloom_effect;
	renderBloomPostProcess.Cleanup = cleanup_bloom_effect;
	renderBloomPostProcess.Draw = draw_bloom_effect;
	return &renderBloomPostProcess;
}
