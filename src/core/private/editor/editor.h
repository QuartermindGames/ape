// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_public_editor.h"

PL_EXTERN_C

typedef struct ApeEditorModeInterface
{
	bool ( *setup )( ApeEditorInstance *self );
	void ( *cleanup )( ApeEditorInstance *self );
	void ( *drawScene )( ApeEditorInstance *self );
	void ( *drawOverlay )( ApeEditorInstance *self );
	bool ( *save )( ApeEditorInstance *self, const char *path );
	bool ( *load )( ApeEditorInstance *self, const char *path );
} ApeEditorModeInterface;

void ape_initialize_editor_( void );
void ape_shutdown_editor_( void );

/**
 * Creates a new instance of ApeEditorInstance in the specified mode.
 *
 * @param mode The editor mode to initialize the instance with.
 * @return 		A pointer to the newly created ApeEditorInstance.
 *         		If the instance could not be created, returns nullptr.
 */
ApeEditorInstance *ape_editor_instance_create_( ApeEditorMode mode );

void ape_editor_register_console_( void );

void ape_editor_pre_render_scene_( ApeCamera *camera );

void ape_editor_draw_gui_( const ApeViewport *viewport );
void ape_grid_draw_();

bool ape_is_editor_active( void );

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

ApeViewport *ape_editor_get_selection_viewport_( void );

PL_EXTERN_C_END
