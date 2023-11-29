// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "core_shell.h"//todo: deprecate this
#include "core_scene.h"
#include "core_camera.h"
#include "core_editor.h"

PL_EXTERN_C

bool ss_acl_initialize( unsigned int argc, char **argv, const char *config );
void ss_acl_shutdown( void );

void ss_acl_render_frame( SSArlViewport *viewport );
void ss_acl_tick_frame( void );

struct NdBranch *ss_acl_get_config( void );
struct NdBranch *ss_acl_get_user_config( void );

unsigned int ss_acl_get_num_ticks( void );

bool ss_acl_is_engine_running( void );
bool ss_acl_is_console_open( void );

void ss_acl_input_handle_keyboard_event( int key, unsigned int keyState );
void ss_acl_input_handle_text_event( const char *key );
void ss_acl_input_handle_mouse_button_event( int button, ApeInputState buttonState );
void ss_acl_input_handle_mouse_wheel_event( float x, float y );
void ss_acl_input_handle_mouse_motion_event( int x, int y );

struct GuiPanel *ss_gui_get_root_panel( void );

PL_EXTERN_C_END
