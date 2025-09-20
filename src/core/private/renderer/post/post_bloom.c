// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: BLOOOOOM
// Author:  Mark E. Sowden

#include "post.h"

#include "renderer/material/material.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeShaderProgram *bloomFilterShader;
static ApeShaderProgram *bloomBlurShader;

static ApeRenderTarget *bloomFilterTarget;
static ApeRenderTarget *bloomBlurTarget;

static bool  bloomEnabled;
static float bloomIntensity;
static float bloomThreshold;
static bool  bloomAdditive;

static void register_bloom_console_variables( void )
{
	PlRegisterConsoleVariable( "post_bloom", "Enable/disable bloom effect.", "true", PL_VAR_BOOL, &bloomEnabled, nullptr, true );
	PlRegisterConsoleVariable( "post_bloom.intensity", "Set the intensity of the bloom effect.", "2.0", PL_VAR_F32, &bloomIntensity, nullptr, true );
	PlRegisterConsoleVariable( "post_bloom.threshold", "Sets the threshold of the bloom effect.", "0.10", PL_VAR_F32, &bloomThreshold, nullptr, true );
	PlRegisterConsoleVariable( "post_bloom.additive", "Enable additive blending instead.", "false", PL_VAR_BOOL, &bloomAdditive, nullptr, true );
}

static bool setup_bloom_effect( void )
{
	if ( ( bloomFilterShader = ape_get_shader_by_name( "post_bloom_filter", APE_SHADER_DEFAULT_NULL ) ) == nullptr )
	{
		return false;
	}
	if ( ( bloomBlurShader = ape_get_shader_by_name( "post_bloom_blur", APE_SHADER_DEFAULT_NULL ) ) == nullptr )
	{
		return false;
	}
	if ( ( bloomFilterTarget = ape_render_target_create( "post_bloom", 800, 600, PLG_BUFFER_COLOUR, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR, 0 ) ) == nullptr )
	{
		return false;
	}
	if ( ( bloomBlurTarget = ape_render_target_create( "post_bloom_blur", 800, 600, PLG_BUFFER_COLOUR, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR, 0 ) ) == nullptr )
	{
		return false;
	}
	return true;
}

static void cleanup_bloom_effect( void )
{
	ape_render_target_release( bloomFilterTarget );
	ape_render_target_release( bloomBlurTarget );
}

static void draw_bloom_effect( const ApeViewport *viewport )
{
	if ( !bloomEnabled || viewport->renderTarget == NULL )
	{
		return;
	}

	// this is currently pretty shit due to limitations in the plgraphics implementation,
	// so we have to do it in more passes than we should

	ApeRenderTarget *postRenderTarget = ape_postfx_get_render_target();
	assert( postRenderTarget != nullptr );

	PLGTexture *viewportTexture = ape_render_target_get_texture( postRenderTarget );
	assert( viewportTexture != nullptr );

	// we use the texture here because supersampling madness...
	int bw = ( int ) viewportTexture->w / 2;
	int bh = ( int ) viewportTexture->h / 2;
	ape_setup_2d_viewport_( bw, bh );

	// draw with filter into bloom render target
	{
		ape_render_target_bind( bloomFilterTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_render_target_set_size( bloomFilterTarget, bw, bh );

		ape_shader_set_active_( bloomFilterShader );

		PlgSetShaderUniformValue( bloomFilterShader->internal, "threshold", &bloomThreshold, false );
		PlgSetShaderUniformValue( bloomFilterShader->internal, "intensity", &bloomIntensity, false );
		PlgSetTexture( viewportTexture, 0 );

		ape_draw_textured_quad( nullptr, 0.0f, 0.0f, ( float ) bw, ( float ) bh, &PL_COLOUR_WHITE, 0 );
	}

	// blur
	{
		PLGTexture *bloomFilterTexture = ape_render_target_get_texture( bloomFilterTarget );
		assert( bloomFilterTexture != nullptr );

		ape_render_target_bind( bloomBlurTarget, PLG_FRAMEBUFFER_DRAW );
		ape_render_target_set_size( bloomBlurTarget, bw, bh );

		ape_shader_set_active_( bloomBlurShader );

		PlgSetShaderUniformValue( bloomBlurShader->internal, "viewportSize", &QM_MATH_VECTOR2F( ( float ) bw, ( float ) bh ), false );
		PlgSetTexture( bloomFilterTexture, 0 );

		ape_draw_textured_quad( nullptr, 0.0f, 0.0f, ( float ) bw, ( float ) bh, &PL_COLOUR_WHITE, 0 );
	}

	// final blend
	{
		PLGTexture *bloomBlurTexture = ape_render_target_get_texture( bloomBlurTarget );
		assert( bloomBlurTexture != nullptr );

		ape_setup_2d_viewport_( viewport->width, viewport->height );

		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT );
		ape_render_target_bind( ape_postfx_get_render_target(), PLG_FRAMEBUFFER_DEFAULT );

		if ( bloomAdditive )
		{
			PlgSetBlendMode( PLG_BLEND_ADDITIVE );
		}
		else
		{
			PlgSetBlendMode( PLG_BLEND_ONE, PLG_BLEND_ONE );
		}
		PlgSetTexture( bloomBlurTexture, 0 );

		ape_draw_textured_quad( nullptr, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE, 0 );

		PlgSetBlendMode( PLG_BLEND_DISABLE );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const ApePostProcessEffect *ape_postfx_get_bloom_( void )
{
	static ApePostProcessEffect renderBloomPostProcess;
	PL_ZERO_( renderBloomPostProcess );
	renderBloomPostProcess.registerConsoleVariables = register_bloom_console_variables;
	renderBloomPostProcess.setup                    = setup_bloom_effect;
	renderBloomPostProcess.cleanup                  = cleanup_bloom_effect;
	renderBloomPostProcess.draw                     = draw_bloom_effect;
	return &renderBloomPostProcess;
}
