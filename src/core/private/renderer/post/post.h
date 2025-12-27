// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape_private.h"

#include "../renderer.h"

typedef struct ApePostProcessEffect
{
	void ( *registerConsoleVariables )( void );
	bool ( *setup )( void );
	void ( *cleanup )( void );
	void ( *draw )( const ApeViewport *viewport, const ApeCamera *camera );
} ApePostProcessEffect;

void ape_postfx_cleanup_( void );
void ape_postfx_setup_( void );

void ape_register_postfx_console_variables_( void );

void ape_postfx_draw_( const ApeViewport *viewport, const ApeCamera *camera );

const ApePostProcessEffect *ape_postfx_get_bloom_( void );
const ApePostProcessEffect *ape_postfx_get_fxaa_( void );

ApeRenderTarget *ape_postfx_get_render_target_( void );

bool ape_postfx_is_enabled_();
