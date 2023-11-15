// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>

#include "core_renderer.h"
#include "core_input.h"

/* ======================================================================
 * OS/SHELL INTERFACE
 * TODO: should this really remain part of core?
 * ====================================================================*/

PL_EXTERN_C

#define SS_SHELL_TICK_RATE ( 1000 / 60 ) /* ms */

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
ApeViewport *ss_shell_create_window( const char *title, int width, int height, bool fullscreen, uint8_t mode );
bool ss_shell_set_window_size( int *width, int *height );
void ss_shell_get_window_size( int *width, int *height );
void ss_shell_display_message( SS_Shell_MessageBoxType messageType, const char *message, ... );
void ss_shell_set_window_icon( const PLImage *image );

////////////////////////////////////////////////////////////////////
// Low Level Input
ApeInputState ss_shell_get_button_state( ApeInputButton inputButton );
ApeInputState ss_shell_get_key_state( int key );
void ss_shell_get_mouse_position( int *x, int *y );
void ss_shell_set_mouse_position( int x, int y );
void ss_shell_grab_mouse( bool grab );
void ss_shell_push_message( int level, const char *msg, const PLColour *colour );
void ss_shell_shutdown( void );

PL_EXTERN_C_END
