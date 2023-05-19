// SPDX-License-Identifier: LGPL-3.0-or-later
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

#define YN_CORE_TICK_RATE ( 1000 / 60 ) /* ms */

typedef enum YNCoreMessageType
{
	OGE_MESSAGE_ERROR,
	OGE_MESSAGE_WARNING,
	OGE_MESSAGE_INFO,
} YNCoreMessageType;

enum
{
	OGE_GRAPHICS_SOFTWARE,
	OGE_GRAPHICS_OPENGL,
	OGE_GRAPHICS_VULKAN,
	OGE_GRAPHICS_OTHER,

	YN_CORE_MAX_GRAPHICS_MODES
};

////////////////////////////////////////////////////////////////////
// Window Management
YNCoreViewport *ogeShellInterface_CreateWindow( const char *title, int width, int height, bool fullscreen, uint8_t mode );
bool ogeShellInterface_SetWindowSize( int *width, int *height );
void ogeShellInterface_GetWindowSize( int *width, int *height );
void YnCore_ShellInterface_DisplayMessageBox( YNCoreMessageType messageType, const char *message, ... );

////////////////////////////////////////////////////////////////////
// Low Level Input
YNCoreInputState YnCore_ShellInterface_GetButtonState( YNCoreInputButton inputButton );
YNCoreInputState YnCore_ShellInterface_GetKeyState( int key );
void ogeShellInterface_GetMousePosition( int *x, int *y );
void YnCore_ShellInterface_SetMousePosition( int x, int y );
void YnCore_ShellInterface_GrabMouse( bool grab );
void YnCore_ShellInterface_PushMessage( int level, const char *msg, const PLColour *colour );
void ogeShellInterface_Shutdown( void );

PL_EXTERN_C_END
