// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Post processing handler
// Author:  Mark E. Sowden

#include "post.h"

#include "../renderer_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

enum
{
	POST_EFFECT_FXAA,
	POST_EFFECT_BLOOM,

	MAX_POST_EFFECTS
};

static const SSArlPostProcessEffect *postProcessEffects[ MAX_POST_EFFECTS ];
static bool postProcessInit = false;
static bool postProcessEnabled = true;

static SSArlRenderTarget *ppRenderTarget = NULL;

static void register_post_effects( void )
{
	if ( postProcessInit )
		return;

	PL_ZERO( postProcessEffects, sizeof( SSArlPostProcessEffect * ) * MAX_POST_EFFECTS );

	postProcessEffects[ POST_EFFECT_FXAA ] = ss_arl_postfx_get_fxaa_();
	postProcessEffects[ POST_EFFECT_BLOOM ] = ss_arl_postfx_get_bloom_();

	postProcessInit = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

bool ss_arl_postfx_is_enabled( void )
{
	return postProcessEnabled;
}

void ss_arl_postfx_cleanup_( void )
{
	if ( !postProcessInit )
		return;

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
			continue;

		postProcessEffects[ i ]->Cleanup();
		postProcessEffects[ i ] = NULL;
	}

	postProcessInit = false;

	ss_arl_render_target_release( ppRenderTarget );
}

void ss_arl_postfx_setup_( void )
{
	ppRenderTarget = ss_arl_render_target_create( "postfx",
	                                              800, 600,
	                                              PLG_BUFFER_COLOUR,
	                                              PLG_BUFFER_COLOUR,
	                                              PLG_TEXTURE_FILTER_LINEAR );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
			continue;

		postProcessEffects[ i ]->Setup();
	}
}

void ss_arl_postfx_register_console_variables_( void )
{
	/* urrrughgdshghfhksd, but yeah... */
	register_post_effects();

	PlRegisterConsoleVariable( "postfx", "Toggles post-processing pipeline.", "1", PL_VAR_BOOL, &postProcessEnabled, NULL, true );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
			continue;

		postProcessEffects[ i ]->RegisterConsoleVariables();
	}
}

void ss_arl_postfx_draw_( const SSArlViewport *viewport )
{
	assert( viewport->renderTarget != NULL );
	PLGTexture *baseTexture = ss_arl_render_target_get_texture( viewport->renderTarget );
	if ( baseTexture == NULL )
		return;

	ss_arl_render_target_set_size( ppRenderTarget, viewport->width, viewport->height );
	ss_arl_render_target_bind( ppRenderTarget, PLG_FRAMEBUFFER_DEFAULT );

	if ( !postProcessEnabled )
	{
		PlgDrawTexturedRectangle( viewport->x, viewport->height, viewport->width, -viewport->height, baseTexture );
		return;
	}

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
			continue;

		postProcessEffects[ i ]->Draw( viewport );
	}
}

SSArlRenderTarget *ss_arl_postfx_get_render_target( void )
{
	return ppRenderTarget;
}
