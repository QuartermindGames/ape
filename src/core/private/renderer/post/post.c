// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Post processing handler
// Author:  Mark E. Sowden

#include "post.h"

#include "renderer/renderer_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef enum PostEffect
{
	POST_EFFECT_FXAA,
	POST_EFFECT_DOF,
	POST_EFFECT_BLOOM,
	POST_EFFECT_DITHER,

	MAX_POST_EFFECTS
} PostEffect;

static const ApePostProcessEffect *postProcessEffects[ MAX_POST_EFFECTS ];
static bool                        postProcessInit    = false;
static bool                        postProcessEnabled = true;

static ApeRenderTarget *ppRenderTarget = nullptr;

extern ApePostProcessEffect ape_postDitherEffect_;

static void register_post_effects( void )
{
	if ( postProcessInit )
	{
		return;
	}

	postProcessEffects[ POST_EFFECT_FXAA ]   = ape_postfx_get_fxaa_();
	postProcessEffects[ POST_EFFECT_BLOOM ]  = ape_postfx_get_bloom_();
	postProcessEffects[ POST_EFFECT_DITHER ] = &ape_postDitherEffect_;

	postProcessInit = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

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

	ape_render_target_release( ppRenderTarget );
}

void ape_postfx_setup_( void )
{
	ppRenderTarget = ape_render_target_create( "postfx",
	                                           800, 600,
	                                           PLG_BUFFER_COLOUR,
	                                           PLG_BUFFER_COLOUR,
	                                           PLG_TEXTURE_FILTER_LINEAR, 0 );
	if ( ppRenderTarget == NULL )
	{
		ape_error_( true, "Failed to create postfx render target: %s\n", PlGetError() );
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

	PlRegisterConsoleVariable( "postfx", "Toggles post-processing pipeline.", "1", PL_VAR_BOOL, &postProcessEnabled, NULL, true );

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
	assert( viewport->renderTarget != nullptr );
	PLGTexture *baseTexture = ape_render_target_get_texture( viewport->renderTarget );
	if ( baseTexture == nullptr )
	{
		return;
	}

	ape_render_target_set_size( ppRenderTarget, viewport->width, viewport->height );

	PLGFrameBuffer *src = ape_render_target_get_frame_buffer( viewport->renderTarget );
	PLGFrameBuffer *dst = ape_render_target_get_frame_buffer( ppRenderTarget );
	PlgBlitFrameBuffers( src, src->width, src->height, dst, viewport->width, viewport->height, true );

	if ( !postProcessEnabled )
	{
		return;
	}

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		ape_render_target_bind( nullptr, PLG_FRAMEBUFFER_DEFAULT );

		if ( postProcessEffects[ i ] == nullptr )
		{
			continue;
		}

		postProcessEffects[ i ]->draw( viewport );
	}
}

ApeRenderTarget *ape_postfx_get_render_target( void )
{
	return ppRenderTarget;
}
