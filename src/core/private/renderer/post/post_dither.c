// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Dithering, for that retro look.
// Author:  Mark E. Sowden

#include "post.h"

#include "renderer/material/material.h"

static bool ditherEnabled;
static bool ditherDownscale;

static ApeShaderProgram *ditherFilterShader;
static ApeRenderTarget  *ditherFilterTarget;

static void register_dither_console_variables()
{
	PlRegisterConsoleVariable( "post_dither", "Enable/disable dithering effect.", "true", PL_VAR_BOOL, &ditherEnabled, nullptr, true );
	PlRegisterConsoleVariable( "post_dither.downscale", "Draw the dithering effect into a smaller resolution buffer.", "false", PL_VAR_BOOL, &ditherDownscale, nullptr, true );
}

static bool setup_dither_effect()
{
	if ( ( ditherFilterShader = ape_get_shader_by_name( "post_dither", APE_SHADER_DEFAULT_NULL ) ) == nullptr )
	{
		return false;
	}
	if ( ( ditherFilterTarget = ape_render_target_create( "post_dither", 800, 600, PLG_BUFFER_COLOUR, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_NEAREST, 0 ) ) == nullptr )
	{
		return false;
	}

	return true;
}

static void cleanup_dither_effect()
{
	ape_render_target_release( ditherFilterTarget );
}

static void draw_dither_effect( const ApeViewport *viewport )
{
	if ( !ditherEnabled )
	{
		return;
	}

	ApeRenderTarget *postRenderTarget = ape_postfx_get_render_target();
	assert( postRenderTarget != nullptr );

	PLGTexture *viewportTexture = ape_render_target_get_texture( postRenderTarget );
	assert( viewportTexture != nullptr );

	{
		int bw, bh;
		if ( ditherDownscale )
		{
			bw = ( int ) viewportTexture->w / 2;
			bh = ( int ) viewportTexture->h / 2;
		}
		else
		{
			bw = viewport->width;
			bh = viewport->height;
		}

		ape_set_2d_viewport_size_( bw, bh );

		ape_render_target_bind( ditherFilterTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_render_target_set_size( ditherFilterTarget, bw, bh );

		ape_shader_set_active_( ditherFilterShader );

		PlgSetTexture( viewportTexture, 0 );
		ape_draw_textured_quad( nullptr, 0.0f, 0.0f, ( float ) bw, ( float ) bh, &PL_COLOUR_WHITE );
	}
	{
		PLGTexture *ditherTexture = ape_render_target_get_texture( ditherFilterTarget );
		assert( ditherTexture != nullptr );

		ape_set_2d_viewport_size_( viewport->width, viewport->height );
		ape_render_target_bind( postRenderTarget, PLG_FRAMEBUFFER_DEFAULT );

		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT );
		PlgSetTexture( ditherTexture, 0 );
		ape_draw_textured_quad( nullptr, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE );
	}
}

ApePostProcessEffect ape_postDitherEffect_ = {
        .registerConsoleVariables = register_dither_console_variables,
        .setup                    = setup_dither_effect,
        .cleanup                  = cleanup_dither_effect,
        .draw                     = draw_dither_effect,
};
