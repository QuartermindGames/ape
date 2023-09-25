// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "ape_private.h"

#include "../renderer.h"

typedef struct ArPostProcessEffect
{
	void ( *RegisterConsoleVariables )( void );
	bool ( *Setup )( void );
	void ( *Cleanup )( void );
	void ( *Draw )( const ApeViewport *viewport );
} ArPostProcessEffect;

void ar_postfx_cleanup_( void );
void ar_postfx_setup_( void );

void R_PP_RegisterConsoleVariables( void );

void ar_postfx_draw_( const ApeViewport *viewport );

const ArPostProcessEffect *ar_postfx_get_bloom_( void );
const ArPostProcessEffect *ar_postfx_get_fxaa_( void );

ArRenderTarget *ar_postfx_get_render_target( void );
