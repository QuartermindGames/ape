// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Primary code for dealing with editor functionality.

#include "ape_private.h"

#include "editor.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_font.h"

#include "game/game_interface.h"
#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeEditorState editorState = {};

static const unsigned int DEFAULT_GRID_SCALE = 2;

static bool gridVisible = true;
static unsigned int gridScale = DEFAULT_GRID_SCALE;
static PLMatrix4 gridTransform;

static ApeRenderTarget *selectionRenderTarget;

static void toggle_grid_command( unsigned int, char ** )
{
	gridVisible = !gridVisible;
}

static void toggle_editor_command( unsigned int, char ** )
{
	ape_config_.editor = !ape_config_.editor;
}

static void save_world_command( unsigned int argc, char **argv )
{
	if ( !ape_is_editor_active() )
	{
		return;
	}

	ApeWorld *world = ss_game_get_current_world();
	if ( world == NULL )
	{
		PRINT_WARNING( "No active world, can't save!\n" );
		return;
	}

	const char *dataPath = com_get_local_data_directory();

	NdBranch *root = nd_branch_push_back_object( NULL, "world" );

	ape_world_serialize_( world, root );
}

static void create_world_command( unsigned int, char ** )
{
	if ( !ape_is_editor_active() )
	{
		return;
	}

	ApeWorld *world = ss_game_get_current_world();
	if ( world != NULL )
	{
		PRINT_WARNING( "Please unload current world before creating another!\n" );
		return;
	}

	world = ape_world_create();

	ss_game_spawn_world( world );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_editor_( void )
{
}

void ape_shutdown_editor_( void )
{
	if ( selectionRenderTarget != NULL )
	{
		ape_render_target_release( selectionRenderTarget );
	}
}

ApeEditorState *ape_editor_get_state( void )
{
	return &editorState;
}

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

void ape_increase_grid_size( void )
{
	gridScale += 2;
}

void ape_decrease_grid_size( void )
{
	gridScale -= 2;

	if ( gridScale == 0 )
	{
		gridScale = 1;
	}
}

unsigned int ape_get_grid_size( void )
{
	return gridScale;
}

void ape_grid_set_visibility( bool visible )
{
	gridVisible = visible;
}

void ape_editor_draw_grid_( void )
{
	if ( !ape_config_.editor || !gridVisible )
	{
		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );//TODO: should probably be view matrix...
	PlPushMatrix();
	PlLoadIdentityMatrix();

	PlRotateMatrix( PL_DEG2RAD( 90.0f ), 1.0f, 0.0f, 0.0f );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	int m = 128;
	PlgDrawGrid( -m / 2, -m / 2, m, m, gridScale / 2, &( PLColour ){ 0, 0, 100, 255 } );
	PlgDrawGrid( -m / 2, -m / 2, m, m, gridScale, &( PLColour ){ 0, 0, 255, 255 } );

	PlPopMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_register_editor_console_variables_( void )
{
	PlRegisterConsoleVariable( "grid_scale", "Scale of the editing grid.", "2", PL_VAR_I32, &gridScale, NULL, true );

	PlRegisterConsoleCommand( "editor", "Toggle main editor functionality.", 0, toggle_editor_command );
	PlRegisterConsoleCommand( "toggle_grid", "Toggle the editing grid.", 0, toggle_grid_command );

	PlRegisterConsoleCommand( "editor_save_world", "Save the current level with the specified name.", 1, save_world_command );
	PlRegisterConsoleCommand( "editor_create_world", "Create a new world instance.", 0, create_world_command );
}

void ape_editor_draw_gui_( const ApeViewport *viewport )
{
	if ( !ape_is_editor_active() )
	{
		return;
	}

	ApeCamera *camera = viewport->camera;
	if ( camera == NULL )
	{
		return;
	}

	char label[ 64 ] = {};
	strcat( label, ape_get_camera_draw_mode_label( camera->drawMode ) );
	strcat( label, " / " );
	strcat( label, ape_get_camera_view_mode_label( camera->mode ) );

	ApeBitmapFont *font = ape_get_default_small_bitmap_font();
	ape_bitmap_font_draw_string( font,
	                             ( float ) ( ( viewport->width - ( font->cw * 2 ) ) - ( font->cw * strlen( label ) ) ),
	                             ( float ) ( viewport->height - ( font->ch * 2 ) ),
	                             1.0f, 1.0f, PL_COLOUR_GOLD, label, true );

	if ( camera->mode != APE_CAMERA_MODE_INVALID && camera->mode != APE_CAMERA_MODE_PERSPECTIVE )
	{
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

		float z = viewport->zoom;
		int zoom = roundl( z ) / 2;
		if ( zoom <= 0 )
		{
			zoom = 1;
		}

		int x = 500 + sinl( zoom * 2 ) * 100.0f;
		int y = 200 + cosl( zoom * 2 ) * 100.0f;

		PlMatrixMode( PL_MODELVIEW_MATRIX );//TODO: should probably be view matrix...
		PlPushMatrix();
		PlLoadIdentityMatrix();

		PlgDrawGrid( 0, 0, viewport->width, viewport->height, ( gridScale / 2 ) * zoom, &( PLColour ){ 0, 0, 100, 255 } );
		PlgDrawGrid( 0, 0, viewport->width, viewport->height, gridScale * zoom, &( PLColour ){ 0, 0, 255, 255 } );

		switch ( camera->mode )
		{
			default:
				break;
			case APE_CAMERA_MODE_TOP:
				PlTranslateMatrix( ( PLVector3 ){ x, -0.0f, -y } );
				PlRotateMatrix( PL_DEG2RAD( 90.0f ), 1.0f, 0.0f, 0.0f );
				break;
			case APE_CAMERA_MODE_LEFT:
				PlTranslateMatrix( ( PLVector3 ){ 0.0f, -y, -x } );
				PlRotateMatrix( PL_DEG2RAD( 90.0f ), 0.0f, 1.0f, 0.0f );
				PlRotateMatrix( PL_DEG2RAD( 180.0f ), 0.0f, 0.0f, 1.0f );
				break;
			case APE_CAMERA_MODE_FRONT:
				PlTranslateMatrix( ( PLVector3 ){ -x, -y, 0.0f } );
				PlRotateMatrix( PL_DEG2RAD( 180.0f ), 0.0f, 0.0f, 1.0f );
				break;
		}

		PlgSetViewMatrix( PlGetMatrix( PL_VIEW_MATRIX ) );

		ApeWorld *world = ss_game_get_current_world();
		if ( world != NULL )
		{
			switch ( camera->drawMode )
			{
				case APE_CAMERA_DRAW_MODE_WIREFRAME:
					ape_world_draw_wireframe( world, camera );
					break;
				case APE_CAMERA_DRAW_MODE_SOLID:
				case APE_CAMERA_DRAW_MODE_TEXTURED:
					ape_world_draw( world, camera, NULL, 0 );
					break;
				default:
					break;
			}
		}

		PlPopMatrix();

		// Restore the view matrix back
		PlgSetViewMatrix( &viewport->camera->internal->internal.view );
	}
}

bool ape_is_editor_active( void )
{
	return ape_config_.editor;
}
