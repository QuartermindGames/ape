// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core_shell.h"
#include "core_scene.h"
#include "core_camera.h"
#include "core_editor.h"

PL_EXTERN_C

bool ape_initialize( unsigned int argc, char **argv, const char *config );
void ape_shutdown( void );

void ape_render_frame( ApeViewport *viewport );
void ape_tick_frame( void );

struct NdBranch *ss_acl_get_config( void );
struct NdBranch *ss_acl_get_user_config( void );

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
