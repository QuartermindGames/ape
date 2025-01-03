// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "core_shell.h"
#include "core_scene.h"
#include "core_camera.h"

#include "ape/ape_public_editor.h"

PL_EXTERN_C

typedef unsigned long int  ulong;
typedef unsigned short int ushort;
typedef unsigned int       uint;

#define APE_DEFAULT_TICK_RATE ( 1000 / 60 )// ms

#define APE_SELF_CAST( X, Y ) ( ( X * ) ( Y ) )

bool ape_initialize( unsigned int argc, char **argv, const char *config, bool embedded );
void ape_shutdown( void );

void ape_render_frame( ApeViewport *viewport );
void ape_tick_frame( void );

struct AcmBranch *ape_get_config( void );
struct AcmBranch *ape_get_user_config( void );

unsigned int ape_get_num_ticks( void );

bool ape_is_running( void );
bool ape_is_console_open( void );

void ape_input_handle_keyboard_event( int key, bool isPressed );
void ape_input_handle_text_event( const char *key );
void ape_input_handle_mouse_button_event( int button, ApeInputState buttonState );
void ape_input_handle_mouse_wheel_event( float x, float y );
void ape_input_handle_mouse_motion_event( int x, int y );

struct GuiPanel *ss_gui_get_root_panel( void );

PL_EXTERN_C_END
