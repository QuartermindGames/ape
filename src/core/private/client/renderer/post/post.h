// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "ape_private.h"

#include "../renderer.h"

typedef struct SSArlPostProcessEffect
{
	void ( *RegisterConsoleVariables )( void );
	bool ( *Setup )( void );
	void ( *Cleanup )( void );
	void ( *Draw )( const ApeViewport *viewport );
} SSArlPostProcessEffect;

bool ss_arl_postfx_is_enabled( void );

void ss_arl_postfx_cleanup_( void );
void ss_arl_postfx_setup_( void );

void ss_arl_postfx_register_console_variables_( void );

void ape_postfx_draw_( const ApeViewport *viewport );

const SSArlPostProcessEffect *ss_arl_postfx_get_bloom_( void );
const SSArlPostProcessEffect *ss_arl_postfx_get_fxaa_( void );

ApeRenderTarget *ape_postfx_get_render_target( void );
