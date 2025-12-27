// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "core_shell.h"
#include "core_scene.h"

#include "ape/ape_public_camera.h"
#include "ape/ape_public_editor.h"

PL_EXTERN_C

typedef unsigned long int  ulong;
typedef unsigned short int ushort;
typedef unsigned int       uint;

#define APE_DEFAULT_TICK_RATE ( 1000 / 60 )// ms

bool ape_initialize( unsigned int argc, char **argv, const char *config );
void ape_shutdown( void );

void ape_render_frame( ApeViewport *viewport );
void ape_tick_frame();

uint64_t ape_get_num_ticks( void );

/**
 * Check if the engine is operating in a dedicated terminal-only mode, so
 * essentially configured for a dedicated server.
 *
 * @return True if the engine is operating in a dedicated terminal-only mode.
 */
bool ape_is_dedicated();

bool ape_is_running( void );
bool ape_is_console_open( void );

void ape_input_handle_keyboard_event( int key, bool isPressed );
void ape_input_handle_text_event( const char *key );
void ape_input_handle_mouse_button_event( int button, ApeInputState buttonState );
void ape_input_handle_mouse_wheel_event( float x, float y );
void ape_input_handle_mouse_motion_event( int x, int y );

PL_EXTERN_C_END
