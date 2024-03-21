// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_public_editor.h"

PL_EXTERN_C

void ape_initialize_editor_( void );
void ape_shutdown_editor_( void );

void ape_register_editor_console_variables_( void );

void ape_editor_pre_render_scene_( const ApeCamera *camera );

void ape_editor_draw_gui_( const ApeViewport *viewport );
void ape_grid_draw_( ApeCamera *camera );

bool ape_is_editor_active( void );

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

ApeViewport *get_selection_viewport_( void );

PL_EXTERN_C_END
