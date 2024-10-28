// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_public_editor.h"

PL_EXTERN_C

typedef struct ApeEditorModeInterface
{
	void ( *initialize )();// global initialisation for the mode
	void ( *shutdown )();  // global shutdown for the mode

	bool ( *setup )( ApeEditorInstance *self );                 // setup instance for mode
	void ( *cleanup )( ApeEditorInstance *self );               // cleanup instance for mode
	void ( *drawScene )( ApeEditorInstance *self );             // draw the 3d scene for mode
	void ( *drawOverlay )( ApeEditorInstance *self );           // draw the 2d scene for mode
	void ( *tick )( ApeEditorInstance *self );                  // tick mode
	bool ( *save )( ApeEditorInstance *self, const char *path );// save current instance data
	bool ( *load )( ApeEditorInstance *self, const char *path );// load current instance data
} ApeEditorModeInterface;

void ape_initialize_editor_( void );
void ape_shutdown_editor_( void );

ApeEditorInstance *ape_editor_instance_create_( ApeEditorMode mode );

void ape_editor_register_console_( void );

void ape_editor_pre_render_scene_( ApeCamera *camera );
void ape_editor_post_render_scene_();

void ape_editor_draw_gui_( const ApeViewport *viewport );

void ape_grid_draw_( const ApeEditorGrid *self );
void ape_grid_post_draw_( const ApeEditorGrid *self );

bool ape_is_editor_active( void );

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

ApeViewport *ape_editor_selection_get_viewport_( void );

void ape_editor_selection_initialize_();
void ape_editor_selection_shutdown_();
void ape_editor_selection_rebuild_( ApeEditorInstance *self );
void ape_editor_selection_render_( ApeEditorInstance *self );

PL_EXTERN_C_END
