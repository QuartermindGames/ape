// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "../ape/ape_public_renderer.h"

#include "core_input.h"

/* ======================================================================
 * OS/SHELL INTERFACE
 * TODO: should this really remain part of core?
 * ====================================================================*/

PL_EXTERN_C

typedef enum SS_Shell_MessageBoxType
{
	SS_SHELL_MESSAGE_BOX_TYPE_ERROR,
	SS_SHELL_MESSAGE_BOX_TYPE_WARNING,
	SS_SHELL_MESSAGE_BOX_TYPE_INFO,
} SS_Shell_MessageBoxType;

enum
{
	SS_SHELL_GRAPHICS_MODE_SOFTWARE,
	SS_SHELL_GRAPHICS_MODE_OPENGL,
	SS_SHELL_GRAPHICS_MODE_VULKAN,
	SS_SHELL_GRAPHICS_MODE_OTHER,
};

////////////////////////////////////////////////////////////////////
// Window Management

void  shell_get_window_size( int *width, int *height );
void  shell_display_message( SS_Shell_MessageBoxType messageType, const char *message, ... );
float shell_get_display_scale();

/**
 * Returns a handle to the currently active viewport.
 */
ApeViewport *ss_shell_viewport_get_active( void );

////////////////////////////////////////////////////////////////////
// Low Level Input
ApeInputState ss_shell_get_button_state( ApeInputButton inputButton );
ApeInputState ss_shell_get_key_state( int key );
void          shell_set_mouse_position( int x, int y );
void          ss_shell_grab_mouse( bool grab );

void ss_shell_shutdown( void );

PL_EXTERN_C_END
