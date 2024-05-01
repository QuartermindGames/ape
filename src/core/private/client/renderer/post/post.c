// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Post processing handler
// Author:  Mark E. Sowden

#include "post.h"

#include "../renderer_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef enum PostEffect
{
	POST_EFFECT_FXAA,
	POST_EFFECT_BLOOM,

	MAX_POST_EFFECTS
} PostEffect;

static const ApePostProcessEffect *postProcessEffects[ MAX_POST_EFFECTS ];
static bool postProcessInit = false;
static bool postProcessEnabled = true;

static ApeRenderTarget *ppRenderTarget = NULL;

static void register_post_effects( void )
{
	if ( postProcessInit )
	{
		return;
	}

	PL_ZERO( postProcessEffects, sizeof( ApePostProcessEffect * ) * MAX_POST_EFFECTS );

	postProcessEffects[ POST_EFFECT_FXAA ] = ape_postfx_get_fxaa_();
	postProcessEffects[ POST_EFFECT_BLOOM ] = ape_postfx_get_bloom_();

	postProcessInit = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

bool ape_postfx_is_enabled( void )
{
	return postProcessEnabled;
}

void ape_postfx_cleanup_( void )
{
	if ( !postProcessInit )
	{
		return;
	}

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
		{
			continue;
		}

		postProcessEffects[ i ]->cleanup();
		postProcessEffects[ i ] = NULL;
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
	                                           PLG_TEXTURE_FILTER_LINEAR );
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
