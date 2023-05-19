// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core_shell.h"//todo: deprecate this
#include "core_camera.h"
#include "core_editor.h"

PL_EXTERN_C

bool ogeInitialize( const char *config );
void ogeShutdown( void );

void ogeRenderFrame( YNCoreViewport *viewport );
void ogeTickFrame( void );

unsigned int YnCore_GetNumTicks( void );

bool ogeIsEngineRunning( void );

void ogeHandleKeyboardEvent( int key, unsigned int keyState );
void ogeHandleTextEvent( const char *key );
void ogeHandleMouseButtonEvent( int button, YNCoreInputState buttonState );
void ogeHandleMouseWheelEvent( float x, float y );
void ogeHandleMouseMotionEvent( int x, int y );

PL_EXTERN_C_END
