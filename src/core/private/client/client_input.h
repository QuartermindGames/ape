// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

#include <yin/core_input.h>

typedef struct NdBranch NdBranch;

typedef void ( *ClientInputActionCallback )( YNCoreInputState state );

void Client_Input_Initialize( void );
void Client_Input_Shutdown( void );

void Client_Input_SerializeConfig( NdBranch *root );
void Client_Input_DeserializeConfig( NdBranch *root );

void Client_Input_ClearDevices( void );

void Client_Input_HandleKeyboardEvent( int key, YNCoreInputState keyState );
void Client_Input_HandleMouseButtonEvent( int button, YNCoreInputState buttonState );
void Client_Input_HandleMouseWheelEvent( float x, float y );
void Client_Input_HandleMouseMotionEvent( int x, int y );

void Client_Input_GetMousePosition( int *x, int *y );
void Client_Input_GetMouseDelta( int *x, int *y );

void YnCore_Input_RegisterAction( const char *id,
                                  YNCoreInputButton buttons[], unsigned int numDefaultButtons,
                                  YNCoreInputKey keys[], unsigned int numDefaultKeys,
                                  ClientInputActionCallback actionCallback );
YNCoreInputState Client_Input_GetActionState( const char *id );

void Client_Input_BeginFrame( void );
void Client_Input_Tick( void );
void Client_Input_EndFrame( void );

PL_EXTERN_C_END
