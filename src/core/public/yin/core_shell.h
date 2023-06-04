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

#define APE_TICK_RATE ( 1000 / 60 ) /* ms */

typedef enum ApeMessageBoxType
{
	OGE_MESSAGE_ERROR,
	OGE_MESSAGE_WARNING,
	OGE_MESSAGE_INFO,
} ApeMessageBoxType;

enum
{
	APE_GRAPHICS_SOFTWARE,
	APE_GRAPHICS_OPENGL,
	APE_GRAPHICS_VULKAN,
	APE_GRAPHICS_OTHER,

	APE_MAX_GRAPHICS_MODES
};

////////////////////////////////////////////////////////////////////
// Window Management
ApeViewport *apeShellInterface_CreateWindow( const char *title, int width, int height, bool fullscreen, uint8_t mode );
bool apeShellInterface_SetWindowSize( int *width, int *height );
void apeShellInterface_GetWindowSize( int *width, int *height );
void YnCore_ShellInterface_DisplayMessageBox( ApeMessageBoxType messageType, const char *message, ... );

////////////////////////////////////////////////////////////////////
// Low Level Input
ApeInputState apeShellInterface_GetButtonState( ApeInputButton inputButton );
ApeInputState apeShellInterface_GetKeyState( int key );
void apeShellInterface_GetMousePosition( int *x, int *y );
void apeShellInterface_SetMousePosition( int x, int y );
void apeShellInterface_GrabMouse( bool grab );
void apeShellInterface_PushMessage( int level, const char *msg, const PLColour *colour );
void apeShellInterface_Shutdown( void );

PL_EXTERN_C_END
