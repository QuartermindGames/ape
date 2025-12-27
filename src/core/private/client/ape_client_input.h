// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

#include <yin/core_input.h>
#include <stdbool.h>

typedef struct AcmBranch AcmBranch;

void ape_input_initialize_( void );
void ape_shutdown_input_( void );

void ape_serialize_input_config_( AcmBranch *root );
void ape_deserialize_input_config_( AcmBranch *root );

void ape_clear_input_devices( void );

void ape_client_input_handle_key_event_( int keyIndex, bool isPressed );
void ape_client_input_handle_mouse_button_event_( int button, ApeInputState buttonState );
void ape_client_input_handle_mouse_wheel_event( float x, float y );
void Client_Input_HandleMouseMotionEvent( int x, int y );

void ape_begin_input_frame_( void );
void ape_input_tick_( void );
void ape_end_input_frame_( void );
void ape_input_center_mouse( void );

PL_EXTERN_C_END
