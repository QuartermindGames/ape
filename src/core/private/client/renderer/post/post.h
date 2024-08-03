// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape_private.h"

#include "../renderer.h"

typedef struct ApePostProcessEffect
{
	void ( *registerConsoleVariables )( void );
	bool ( *setup )( void );
	void ( *cleanup )( void );
	void ( *draw )( const ApeViewport *viewport );
} ApePostProcessEffect;

void ape_postfx_cleanup_( void );
void ape_postfx_setup_( void );

void ape_register_postfx_console_variables_( void );

void ape_postfx_draw_( const ApeViewport *viewport );

const ApePostProcessEffect *ape_postfx_get_bloom_( void );
const ApePostProcessEffect *ape_postfx_get_fxaa_( void );

ApeRenderTarget *ape_postfx_get_render_target( void );
