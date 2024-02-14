// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

#include <yin/core_input.h>

typedef struct NdBranch NdBranch;

void ape_initialize_input_( void );
void apeShutdownInput_( void );

void apeSerializeInputConfig_( NdBranch *root );
void apeDeserializeInputConfig_( NdBranch *root );

void apeClearInputDevices_( void );

void Client_Input_HandleKeyboardEvent( int key, ApeInputState keyState );
void Client_Input_HandleMouseButtonEvent( int button, ApeInputState buttonState );
void Client_Input_HandleMouseWheelEvent( float x, float y );
void Client_Input_HandleMouseMotionEvent( int x, int y );

void ape_begin_input_frame_( void );
void ape_tick_input_( void );
void ape_end_input_frame_( void );
void acl_input_center_mouse( void );

PL_EXTERN_C_END
