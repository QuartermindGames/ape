// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "ape_private.h"

#include "../renderer.h"

typedef struct ArPostProcessEffect
{
	void ( *RegisterConsoleVariables )( void );
	bool ( *Setup )( void );
	void ( *Cleanup )( void );
	void ( *Draw )( const SS_Arl_Viewport *viewport );
} ArPostProcessEffect;

void arl_postfx_cleanup_( void );
void arl_postfx_setup_( void );

void ss_arl_postfx_register_console_variables_( void );

void arl_postfx_draw_( const SS_Arl_Viewport *viewport );

const ArPostProcessEffect *arl_postfx_get_bloom_( void );
const ArPostProcessEffect *arl_postfx_get_fxaa_( void );

ArRenderTarget *arl_postfx_get_render_target( void );
