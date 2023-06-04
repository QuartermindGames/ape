// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

#include <yin/core_input.h>

typedef struct NdBranch NdBranch;

void apeInitializeInput_( void );
void apeShutdownInput_( void );

void apeSerializeInputConfig_( NdBranch *root );
void apeDeserializeInputConfig_( NdBranch *root );

void apeClearInputDevices_( void );

void Client_Input_HandleKeyboardEvent( int key, OgeInputState keyState );
void Client_Input_HandleMouseButtonEvent( int button, OgeInputState buttonState );
void Client_Input_HandleMouseWheelEvent( float x, float y );
void Client_Input_HandleMouseMotionEvent( int x, int y );

void apeBeginInputFrame_( void );
void apeTickInput_( void );
void apeEndInputFrame_( void );

PL_EXTERN_C_END
