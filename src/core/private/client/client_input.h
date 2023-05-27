// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

#include <yin/core_input.h>

typedef struct NdBranch NdBranch;

void ogeInitializeInput_( void );
void ogeShutdownInput_( void );

void ogeSerializeInputConfig_( NdBranch *root );
void ogeDeserializeInputConfig_( NdBranch *root );

void ogeClearInputDevices_( void );

void Client_Input_HandleKeyboardEvent( int key, OgeInputState keyState );
void Client_Input_HandleMouseButtonEvent( int button, OgeInputState buttonState );
void Client_Input_HandleMouseWheelEvent( float x, float y );
void Client_Input_HandleMouseMotionEvent( int x, int y );

void ogeBeginInputFrame_( void );
void ogeTickInput_( void );
void ogeEndInputFrame_( void );

PL_EXTERN_C_END
