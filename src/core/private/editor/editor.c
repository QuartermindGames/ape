// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Primary code for dealing with editor functionality.

#include "ape_private.h"

#include "plcore/pl_hashtable.h"

#include "editor.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_font.h"

#include "game/game_interface.h"
#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeEditorState editorState = {};

//static bool gridVisible = true;
//static unsigned int gridScale = DEFAULT_GRID_SCALE;

static ApeViewport *selectionViewport;
static PLHashTable *selectionObjectTable;

static void toggle_grid_command( unsigned int, char ** )
{
	editorState.gridVisible = !editorState.gridVisible;
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

static void grid_initialize( void );
static void grid_shutdown( void );

void ape_initialize_editor_( void )
{
	editorState.geometryMode = APE_EDITOR_GEOMETRY_MODE_BRUSH;

	selectionObjectTable = PlCreateHashTable();

	selectionViewport = ape_viewport_create( 0, 0, 640, 480, NULL );
	if ( selectionViewport == NULL )
	{
		ape_error_( true, "Failed to create selection viewport!\n" );
	}

	grid_initialize();
}

void ape_shutdown_editor_( void )
{
	PlDestroyHashTable( selectionObjectTable );

	ape_viewport_destroy( selectionViewport );

	grid_shutdown();
}

ApeEditorState *ape_editor_get_state( void )
{
	return &editorState;
}

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

static const unsigned int DEFAULT_GRID_SCALE = 2;
static const unsigned int MAX_GRID_SCALE = 16;
#define GRID_SIZE     64
#define GRID_ELEMENTS ( GRID_SIZE * GRID_SIZE )

static PLMatrix4 gridTransform;

typedef struct GridSelectable
{
	PLColour colour;
	PLVector3 position;
} GridSelectable;
static GridSelectable gridSelectables[ GRID_ELEMENTS ];
static GridSelectable *activeGridSelectable;
static PLHashTable *gridSelectablesTable;

static unsigned int gridOldScale = DEFAULT_GRID_SCALE;

static const float GRID_SELECTABLE_SCALE = 0.5f;

static void grid_update_selection_points( void );

static void grid_initialize( void )
{
	editorState.gridVisible = true;
	editorState.gridScale = DEFAULT_GRID_SCALE;

	gridTransform = PlMatrix4Identity();
	PLMatrix4 gridRotation = PlRotateMatrix4( PL_DEG2RAD( 90.0f ), &( PLVector3 ){ 1.0f, 0.0f, 0.0f } );
	gridTransform = PlMultiplyMatrix4( gridTransform, &gridRotation );

	gridSelectablesTable = PlCreateHashTable();

	// assign colours to each of the selection cubes
	unsigned int aaa = 1;
	for ( unsigned int i = 0; i < GRID_ELEMENTS; ++i )
	{
		gridSelectables[ i ].colour.r = aaa & 0xFF;
		gridSelectables[ i ].colour.g = ( aaa & 0xFF00 ) >> 8;
		gridSelectables[ i ].colour.b = ( aaa & 0xFF0000 ) >> 16;
		gridSelectables[ i ].colour.a = 255;
		aaa += 16;

		PlInsertHashTableNode( gridSelectablesTable, &gridSelectables[ i ].colour, sizeof( PLColour ), &gridSelectables[ i ] );
	}

	grid_update_selection_points();
}

static void grid_shutdown( void )
{
	PlDestroyHashTable( gridSelectablesTable );
	gridSelectablesTable = NULL;
}

static void grid_update_selection_points( void )
{
	for ( unsigned int r = 0; r < GRID_SIZE; ++r )
	{
		for ( unsigned int c = 0; c < GRID_SIZE; ++c )
		{
			GridSelectable *selectable = &gridSelectables[ r * GRID_SIZE + c ];
			selectable->position.x = ( float ) r - ( ( ( float ) GRID_SIZE / 2.0f ) /* + ( GRID_SELECTABLE_SCALE / 2.0f )*/ );
			selectable->position.y = ( float ) c - ( ( ( float ) GRID_SIZE / 2.0f ) /* + ( GRID_SELECTABLE_SCALE / 2.0f )*/ );
		}
	}

	//todo: mesh should also be regenerated here
}

static void grid_batch_selection_point( const ApeCamera *camera, const GridSelectable *selectable )
{
	PLCollisionAABB bounds = ( PLCollisionAABB ){
	        .origin = PlTransformVector3( &selectable->position, &gridTransform ),
	        .maxs = ( PLVector3 ){GRID_SELECTABLE_SCALE,   GRID_SELECTABLE_SCALE,  GRID_SELECTABLE_SCALE },
	        .mins = ( PLVector3 ){ -GRID_SELECTABLE_SCALE, -GRID_SELECTABLE_SCALE, -GRID_SELECTABLE_SCALE}
    };
	if ( !PlgIsBoxInsideView( camera->internal, &bounds ) )
	{
		return;
	}

	float scale = GRID_SELECTABLE_SCALE / 2.0f;

	unsigned int x, y, z, w;
	x = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y + scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	y = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y - scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	z = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y + scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	w = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y - scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );

	PlgImmPushTriangle( x, y, z );
	PlgImmPushTriangle( y, z, w );

	x = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	y = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	z = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	w = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );

	PlgImmPushTriangle( x, y, z );
	PlgImmPushTriangle( y, z, w );

	x = PlgImmPushVertex( selectable->position.x, selectable->position.y + scale, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	y = PlgImmPushVertex( selectable->position.x, selectable->position.y - scale, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	z = PlgImmPushVertex( selectable->position.x, selectable->position.y + scale, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	w = PlgImmPushVertex( selectable->position.x, selectable->position.y - scale, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );

	PlgImmPushTriangle( x, y, z );
	PlgImmPushTriangle( y, z, w );
}

static void update_active_grid_selection( void )
{
	PLGFrameBuffer *frameBuffer = ape_render_target_get_frame_buffer( selectionViewport->renderTarget );
	if ( frameBuffer == NULL )
	{
		return;
	}

	size_t size = frameBuffer->width * frameBuffer->height * 4;
	PLColour *buf = PL_NEW_( PLColour, size );
	if ( PlgReadFrameBufferRegion( frameBuffer, 0, 0, frameBuffer->width, frameBuffer->height, size, buf ) != NULL )
	{
		int x, y;
		ape_client_input_get_mouse_position( &x, &y );

		// selection buffer is half of the source
		x /= 2;
		y /= 2;

		if ( x < frameBuffer->width && y < frameBuffer->height )
		{
			const PLColour *pixel = &buf[ ( frameBuffer->height - y - 1 ) * frameBuffer->width + x ];
			GridSelectable *selectable = PlLookupHashTableUserData( gridSelectablesTable, pixel, sizeof( PLColour ) );
			if ( selectable != NULL )
			{
				activeGridSelectable = selectable;
			}
		}
	}
	else
	{
		ape_warning_( "Failed to read framebuffer: %s\n", PlGetError() );
	}

	PL_DELETE( buf );
}

/**
 * this draws what should be selectable to the selection buffer.
 */
static void draw_selection_grid( ApeCamera *camera )
{
	if ( gridOldScale != editorState.gridScale )
	{
		grid_update_selection_points();
		gridOldScale = editorState.gridScale;
	}

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &gridTransform );

	PlgImmBegin( PLG_MESH_TRIANGLES );
	for ( unsigned int i = 0; i < GRID_ELEMENTS; i++ )
	{
		grid_batch_selection_point( camera, &gridSelectables[ i ] );
	}

	PlgSetCullMode( PLG_CULL_NONE );

	PlgImmDraw();

	PlgSetCullMode( PLG_CULL_POSITIVE );

	PlPopMatrix();
}

PLVector3 ape_grid_get_cursor_position( void )
{
	if ( activeGridSelectable == NULL )
	{
		return pl_vecOrigin3;
	}

	return PlTransformVector3( &activeGridSelectable->position, &gridTransform );
}

void ape_grid_increase_size( void )
{
	editorState.gridScale = PlClamp( DEFAULT_GRID_SCALE, ( editorState.gridScale * 2 ), MAX_GRID_SCALE );
	activeGridSelectable = NULL;
}

void ape_grid_decrease_size( void )
{
	editorState.gridScale = PlClamp( DEFAULT_GRID_SCALE, ( editorState.gridScale / 2 ), MAX_GRID_SCALE );
	activeGridSelectable = NULL;
}

unsigned int ape_grid_get_size( void )
{
	return editorState.gridScale;
}

void ape_grid_set_visibility( bool visible )
{
	editorState.gridVisible = visible;
	activeGridSelectable = NULL;
}

void ape_grid_draw_( ApeCamera *camera )
{
	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == NULL )
	{
		return;
	}

	if ( !ape_config_.editor || !editorState.gridVisible || editorState.gridScale <= 1 )
	{
		return;
	}

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

	if ( editorState.geometryMode == APE_EDITOR_GEOMETRY_MODE_BRUSH )
	{
		unsigned int sw = viewport->width / 2;
		unsigned int sh = viewport->height / 2;
		ape_viewport_set_size( selectionViewport, sw, sh );
		ape_viewport_make_active( selectionViewport );
		ape_render_target_bind( selectionViewport->renderTarget, PLG_FRAMEBUFFER_DRAW );

		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

		//todo: just shove this here for now for testing...
		draw_selection_grid( camera );

		update_active_grid_selection();

		ape_render_target_bind( viewport->renderTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_viewport_make_active( viewport );
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &gridTransform );

	PlgDrawGrid( -GRID_SIZE / 2, -GRID_SIZE / 2, GRID_SIZE, GRID_SIZE, editorState.gridScale / 2, &( PLColour ){ 0, 0, 255, 255 } );

	if ( ( editorState.geometryMode == APE_EDITOR_GEOMETRY_MODE_BRUSH ) && activeGridSelectable != NULL )
	{
		static const float GRID_HIGHLIGHT_SCALE = GRID_SELECTABLE_SCALE / 8.0f;

		PLCollisionAABB bounds = {
		        .origin = activeGridSelectable->position,
		        .mins = {-GRID_HIGHLIGHT_SCALE, -GRID_HIGHLIGHT_SCALE, -GRID_HIGHLIGHT_SCALE},
		        .maxs = { GRID_HIGHLIGHT_SCALE, GRID_HIGHLIGHT_SCALE,  GRID_HIGHLIGHT_SCALE },
		};
		PlgDrawBoundingVolume( &bounds, &( PLColour ){ 255, 255, 255, 255 } );
	}

	PlPopMatrix();
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_register_editor_console_variables_( void )
{
	PlRegisterConsoleVariable( "grid_scale", "Scale of the editing grid.", "2", PL_VAR_I32, &editorState.gridScale, NULL, true );

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

		PlgDrawGrid( 0, 0, viewport->width, viewport->height, ( editorState.gridScale / 2 ) * zoom, &( PLColour ){ 0, 0, 100, 255 } );
		PlgDrawGrid( 0, 0, viewport->width, viewport->height, editorState.gridScale * zoom, &( PLColour ){ 0, 0, 255, 255 } );

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
