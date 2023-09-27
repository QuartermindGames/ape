// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Post processing handler
// Author:  Mark E. Sowden

#include "post.h"

#include "../ar_render_target.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

enum
{
	POST_EFFECT_FXAA,
	POST_EFFECT_BLOOM,

	MAX_POST_EFFECTS
};

static const ArPostProcessEffect *postProcessEffects[ MAX_POST_EFFECTS ];
static bool postProcessInit = false;
static bool postProcessEnabled = true;

static ArRenderTarget *ppRenderTarget = NULL;

static void register_post_effects( void )
{
	if ( postProcessInit )
		return;

	PL_ZERO( postProcessEffects, sizeof( ArPostProcessEffect * ) * MAX_POST_EFFECTS );

	postProcessEffects[ POST_EFFECT_BLOOM ] = ar_postfx_get_bloom_();
	postProcessEffects[ POST_EFFECT_FXAA ] = ar_postfx_get_fxaa_();

	postProcessInit = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ar_postfx_cleanup_( void )
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

	ar_render_target_release( ppRenderTarget );
}

void ar_postfx_setup_( void )
{
	ppRenderTarget = ar_render_target_create( "postfx",
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

void R_PP_RegisterConsoleVariables( void )
{
	/* urrrughgdshghfhksd, but yeah... */
	register_post_effects();

	PlRegisterConsoleVariable( "r/postProcessing", "Toggles post-processing pipeline.", "1", PL_VAR_BOOL, &postProcessEnabled, NULL, true );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
			continue;

		postProcessEffects[ i ]->RegisterConsoleVariables();
	}
}

void ar_postfx_draw_( const ApeViewport *viewport )
{
	if ( !postProcessEnabled )
		return;

	ar_render_target_set_size( ppRenderTarget, viewport->width, viewport->height );
	ar_render_target_bind( ppRenderTarget, PLG_FRAMEBUFFER_DEFAULT );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ] );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, viewport->width, -viewport->height, ar_render_target_get_texture( ar_get_default_render_target() ) );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
			continue;

		postProcessEffects[ i ]->Draw( viewport );
	}

	ar_render_target_bind( ar_get_default_render_target(), PLG_FRAMEBUFFER_DEFAULT );
	PlgDrawTexturedRectangle( viewport->x, viewport->height, viewport->width, -viewport->height, ar_render_target_get_texture( ppRenderTarget ) );
}

ArRenderTarget *ar_postfx_get_render_target( void )
{
	return ppRenderTarget;
}
