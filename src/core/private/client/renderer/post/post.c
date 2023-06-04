/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "post.h"

/****************************************
 * PRIVATE
 ****************************************/

enum
{
	POST_EFFECT_FXAA,
	POST_EFFECT_BLOOM,

	MAX_POST_EFFECTS
};

static const PostProcessEffect *postProcessEffects[ MAX_POST_EFFECTS ];
static bool                     postProcessInit = false;

static PLGFrameBuffer *ppBuffer = NULL;
static PLGTexture     *ppAttachment = NULL;

static void RegisterPostEffects( void )
{
	if ( postProcessInit )
		return;

	PL_ZERO( postProcessEffects, sizeof( PostProcessEffect * ) * MAX_POST_EFFECTS );

	const PostProcessEffect *PP_Bloom_GetEffect( void );
	postProcessEffects[ POST_EFFECT_BLOOM ] = PP_Bloom_GetEffect();

	const PostProcessEffect *PP_FXAA_GetEffect( void );
	postProcessEffects[ POST_EFFECT_FXAA ] = PP_FXAA_GetEffect();

	postProcessInit = true;
}

/****************************************
 * PUBLIC
 ****************************************/

void R_PP_Cleanup( void )
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
}

void R_PP_SetupEffects( void )
{
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
	RegisterPostEffects();

	PlRegisterConsoleVariable( "r.postProcessing", "Toggles post-processing pipeline.", "0", PL_VAR_BOOL, NULL, NULL, true );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
			continue;

		postProcessEffects[ i ]->RegisterConsoleVariables();
	}
}

void R_PP_Draw( const ApeViewport *viewport )
{
	apeSetupRenderTarget( &ppBuffer, &ppAttachment, NULL, viewport->width, viewport->height );

	for ( unsigned int i = 0; i < MAX_POST_EFFECTS; ++i )
	{
		if ( postProcessEffects[ i ] == NULL )
		{
			continue;
		}

		postProcessEffects[ i ]->Draw( viewport );
	}
}
