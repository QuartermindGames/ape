// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core_shell.h"//todo: deprecate this
#include "core_camera.h"
#include "core_editor.h"

PL_EXTERN_C

bool ogeInitialize( const char *config );
void ogeShutdown( void );

void ogeRenderFrame( OgeViewport *viewport );
void ogeTickFrame( void );

struct YNNodeBranch *ogeGetConfig( void );
struct YNNodeBranch *ogeGetUserConfig( void );

unsigned int ogeGetNumTicks( void );

bool ogeIsEngineRunning( void );

void ogeHandleKeyboardEvent( int key, unsigned int keyState );
void ogeHandleTextEvent( const char *key );
void ogeHandleMouseButtonEvent( int button, YNCoreInputState buttonState );
void ogeHandleMouseWheelEvent( float x, float y );
void ogeHandleMouseMotionEvent( int x, int y );

PL_EXTERN_C_END
