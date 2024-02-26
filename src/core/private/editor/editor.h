// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_editor.h>

PL_EXTERN_C

void ape_initialize_editor_( void );
void ape_shutdown_editor_( void );

void ape_register_editor_console_variables_( void );

void ape_editor_draw_gui_( const ApeViewport *viewport );
void ape_editor_draw_grid_( void );

bool ape_is_editor_active( void );

PL_EXTERN_C_END
