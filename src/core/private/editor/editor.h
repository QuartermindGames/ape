// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_public_editor.h"

PL_EXTERN_C

void ape_initialize_editor_( void );
void ape_shutdown_editor_( void );

ApeEditorInstance *ape_editor_instance_create_( ApeEditorMode mode );

void ape_editor_register_console_( void );

void ape_editor_pre_render_scene_( ApeCamera *camera );
void ape_editor_post_render_scene_();

void ape_editor_draw_gui_( const ApeViewport *viewport );

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

void ape_grid_draw_( const ApeEditorGrid *self );
void ape_grid_post_draw_( const ApeEditorGrid *self );

QmMathVector3f ape_grid_update_cursor_( ApeEditorGrid *self, int mx, int my, const ApeCamera *camera, const ApeViewport *viewport );

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

ApeViewport *ape_editor_selection_get_viewport_( void );

void ape_editor_selection_initialize_();
void ape_editor_selection_shutdown_();
void ape_editor_selection_rebuild_( ApeEditorInstance *self );
void ape_editor_selection_render_( ApeEditorInstance *self );
void ape_editor_selection_render_post_( ApeEditorInstance *self );

QmMathColour4ub *ape_editor_selection_get_pixel_under_cursor_( QmMathColour4ub *dst );

/////////////////////////////////////////////////////////////////////////////////////
// Light
/////////////////////////////////////////////////////////////////////////////////////

void ape_editor_light_generate_( ApeRoom *room, bool buildLightmap, bool buildLightGrid );

/////////////////////////////////////////////////////////////////////////////////////
// Interface
/////////////////////////////////////////////////////////////////////////////////////

void ape_editor_ui_initialize_();
void ape_editor_ui_shutdown_();
void ape_editor_ui_draw_( const ApeViewport *viewport );

/////////////////////////////////////////////////////////////////////////////////////

#if !defined( NDEBUG )
/**
 * This iterates over all of the properties and checks if they have a valid type, and typename.
 * It doesn't work outside of debug builds, for performance sake we exclude the typename crap
 * there.
 * @param properties Array of properties.
 * @param numProperties Number of properties in the array.
 * @return True if valid, false otherwise (invalid properties are printed to console).
 */
bool ape_editor_validate_properties_( const ApeProperty *properties, unsigned int numProperties );
#else
#	define ape_editor_validate_properties_( X, Y ) true
#endif

PL_EXTERN_C_END
