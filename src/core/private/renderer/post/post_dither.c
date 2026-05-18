// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Dithering, for that retro look.
// Author:  Mark E. Sowden

#include "post.h"

#include "core_console.h"

#include "renderer/renderer_render_target.h"
#include "renderer/material/material.h"

static bool ditherEnabled;
static bool ditherDownscale;

static ApeShaderProgram *ditherFilterShader;
static ApeRenderTarget  *ditherFilterTarget;

static void register_dither_console_variables()
{
	ape_console_var_register( "post_dither", "Enable/disable dithering effect.", "false", PL_VAR_BOOL, &ditherEnabled, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "post_dither.downscale", "Draw the dithering effect into a smaller resolution buffer.", "false", PL_VAR_BOOL, &ditherDownscale, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
}

static bool setup_dither_effect()
{
	if ( ( ditherFilterShader = ape_get_shader_by_name( "post_dither", APE_SHADER_DEFAULT_NULL ) ) == nullptr )
	{
		return false;
	}
	if ( ( ditherFilterTarget = ape_render_target_create_( "post_dither", 800, 600, PLG_BUFFER_COLOUR, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_NEAREST, 0 ) ) == nullptr )
	{
		return false;
	}

	return true;
}

static void cleanup_dither_effect()
{
	ape_render_target_release_( ditherFilterTarget );
}

static void draw_dither_effect( const ApeViewport *viewport, [[maybe_unused]] const ApeCamera *camera )
{
	if ( !ditherEnabled )
	{
		return;
	}

	ApeRenderTarget *postRenderTarget = ape_postfx_get_render_target_();
	assert( postRenderTarget != nullptr );

	QmGfxTexture *viewportTexture = ape_render_target_get_texture_( postRenderTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR );
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

		ape_setup_2d_viewport_( bw, bh );

		ape_render_target_bind_( ditherFilterTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_render_target_set_size_( ditherFilterTarget, bw, bh );

		ape_shader_set_active_( ditherFilterShader );

		qm_gfx_texture_set( viewportTexture, 0 );

		ape_draw_textured_quad( nullptr, 0.0f, 0.0f, ( float ) bw, ( float ) bh, &PL_COLOUR_WHITE, 0 );

		qm_gfx_texture_set( nullptr, 0 );
	}

	// finalize
	{
		QmGfxTexture *ditherTexture = ape_render_target_get_texture_( ditherFilterTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR );
		assert( ditherTexture != nullptr );

		ape_setup_2d_viewport_( viewport->width, viewport->height );
		ape_render_target_bind_( postRenderTarget, PLG_FRAMEBUFFER_DEFAULT );

		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT );

		qm_gfx_texture_set( ditherTexture, 0 );

		ape_draw_textured_quad( nullptr, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE, 0 );

		qm_gfx_texture_set( nullptr, 0 );
	}
}

ApePostProcessEffect ape_postDitherEffect_ = {
        .registerConsoleVariables = register_dither_console_variables,
        .setup                    = setup_dither_effect,
        .cleanup                  = cleanup_dither_effect,
        .draw                     = draw_dither_effect,
};
