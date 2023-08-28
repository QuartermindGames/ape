// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core_shell.h"//todo: deprecate this
#include "core_scene.h"
#include "core_camera.h"
#include "core_editor.h"

PL_EXTERN_C

bool apeInitialize( const char *config );
void apeShutdown( void );

void apeRenderFrame( ApeViewport *viewport );
void apeTickFrame( void );

struct NdBranch *apeGetConfig( void );
struct NdBranch *apeGetUserConfig( void );

unsigned int apeGetNumTicks( void );

bool apeIsEngineRunning( void );
bool apeIsConsoleOpen( void );

void apeHandleKeyboardEvent( int key, unsigned int keyState );
void apeHandleTextEvent( const char *key );
void apeHandleMouseButtonEvent( int button, ApeInputState buttonState );
void apeHandleMouseWheelEvent( float x, float y );
void apeHandleMouseMotionEvent( int x, int y );

struct GuiPanel *apeGetDefaultRootPanel( void );

PL_EXTERN_C_END
