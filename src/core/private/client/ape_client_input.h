// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

#include <yin/core_input.h>
#include <stdbool.h>

typedef struct NdBranch NdBranch;

void ape_initialize_input_( void );
void ape_shutdown_input_( void );

void ape_serialize_input_config_( NdBranch *root );
void ape_deserialize_input_config_( NdBranch *root );

void ss_ape_clear_input_devices_( void );

void ape_client_input_handle_key_event_( int keyIndex, bool isPressed );
void ape_client_input_handle_mouse_button_event_( int button, ApeInputState buttonState );
void ape_client_input_handle_mouse_wheel_event( float x, float y );
void Client_Input_HandleMouseMotionEvent( int x, int y );

void ape_begin_input_frame_( void );
void ape_tick_input_( void );
void ape_end_input_frame_( void );
void ape_input_center_mouse( void );

PL_EXTERN_C_END
