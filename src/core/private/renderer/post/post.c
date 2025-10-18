// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Post processing handler
// Author:  Mark E. Sowden

#include "post.h"

#include "renderer/renderer_render_target.h"

typedef enum ApeRendererPostEffectType
{
	APE_RENDERER_POST_EFFECT_TYPE_DOF,
	APE_RENDERER_POST_EFFECT_TYPE_FXAA,
	APE_RENDERER_POST_EFFECT_TYPE_BLOOM,
	APE_RENDERER_POST_EFFECT_TYPE_DITHER,

	MAX_POST_EFFECTS
} ApeRendererPostEffectType;

static const ApePostProcessEffect *postProcessEffects[ MAX_POST_EFFECTS ];
static bool                        postProcessInit    = false;
static bool                        postProcessEnabled = true;

static ApeRenderTarget *ppRenderTarget = nullptr;

extern ApePostProcessEffect ape_postDitherEffect_;
extern ApePostProcessEffect ape_postDofEffect_;

static void register_post_effects()
{
	if ( postProcessInit )
	{
		return;
	}

	postProcessEffects[ APE_RENDERER_POST_EFFECT_TYPE_DOF ]    = &ape_postDofEffect_;
	postProcessEffects[ APE_RENDERER_POST_EFFECT_TYPE_FXAA ]   = ape_postfx_get_fxaa_();
	postProcessEffects[ APE_RENDERER_POST_EFFECT_TYPE_BLOOM ]  = ape_postfx_get_bloom_();
	postProcessEffects[ APE_RENDERER_POST_EFFECT_TYPE_DITHER ] = &ape_postDitherEffect_;

	postProcessInit = true;
}

void ape_postfx_cleanup_( void )
{
	if ( !postProcessInit )
	{
		return;
	}

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == nullptr )
		{
			continue;
		}

		postProcessEffects[ i ]->cleanup();
		postProcessEffects[ i ] = nullptr;
	}

	postProcessInit = false;

	ape_render_target_release_( ppRenderTarget );
}

void ape_postfx_setup_( void )
{
	ppRenderTarget = ape_render_target_create_( "postfx",
	                                            800, 600,
	                                            PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH,
	                                            PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH,
	                                            PLG_TEXTURE_FILTER_LINEAR, 0 );
	if ( ppRenderTarget == NULL )
	{
		ape_console_error_( true, "Failed to create postfx render target: %s\n", PlGetError() );
	}

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
		{
			continue;
		}

		postProcessEffects[ i ]->setup();
	}
}

void ape_register_postfx_console_variables_( void )
{
	/* urrrughgdshghfhksd, but yeah... */
	register_post_effects();

	PlRegisterConsoleVariable( "postfx", "Toggles post-processing pipeline.", "true", PL_VAR_BOOL, &postProcessEnabled, nullptr, true );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
		{
			continue;
		}

		postProcessEffects[ i ]->registerConsoleVariables();
	}
}

void ape_postfx_draw_( const ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	assert( viewport->renderTarget != nullptr );

	if ( !postProcessEnabled )
	{
		COM_PROFILE_FUNCTION_END();
		return;
	}

	ape_render_target_set_size_( ppRenderTarget, viewport->width, viewport->height );
	ape_render_target_bind_( ppRenderTarget, PLG_FRAMEBUFFER_DEFAULT );

	PLGFrameBuffer *src = ape_render_target_get_frame_buffer_( viewport->renderTarget );
	PLGFrameBuffer *dst = ape_render_target_get_frame_buffer_( ppRenderTarget );
	PlgBlitFrameBuffers( src, src->width, src->height, dst, viewport->width, viewport->height, PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH, true );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == nullptr )
		{
			continue;
		}

		postProcessEffects[ i ]->draw( viewport );

		ape_render_target_bind_( ppRenderTarget, PLG_FRAMEBUFFER_DEFAULT );
	}

	// bind the viewport render target again
	ape_render_target_bind_( viewport->renderTarget, PLG_FRAMEBUFFER_DEFAULT );

	COM_PROFILE_FUNCTION_END();
}

ApeRenderTarget *ape_postfx_get_render_target_( void )
{
	return ppRenderTarget;
}

bool ape_postfx_is_enabled_()
{
	return postProcessEnabled;
}
