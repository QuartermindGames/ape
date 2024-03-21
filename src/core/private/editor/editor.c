// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Primary code for dealing with editor functionality.

#include "plcore/pl_hashtable.h"

#include "ape_private.h"

#include "editor.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_font.h"
#include "client/renderer/renderer_material.h"

#include "game/game_interface.h"

#include "world/world.h"
#include "yin/gui_public.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeMaterial *planeMaterial;

/////////////////////////////////////////////////////////////////////////////////////
// Editor Instance Management
/////////////////////////////////////////////////////////////////////////////////////

void grid_initialize_( ApeEditorState *instance );
void grid_shutdown_( void );

static ApeEditorState *editorInstance = NULL;

ApeEditorState *ape_editor_instance_initialize( ApeEditorState *self )
{
	PL_ZERO( self, sizeof( ApeEditorState ) );

	self->geometryMode = APE_EDITOR_GEOMETRY_MODE_BRUSH;

	self->brushPlotPoints = PlCreateLinkedList();
	if ( self->brushPlotPoints == NULL )
	{
		ape_warning_( "Failed to create brush plot points list: %s\n", PlGetError() );
		return NULL;
	}

	grid_initialize_( self );

	return self;
}

void ape_editor_instance_shutdown( ApeEditorState *self )
{
	if ( self->brushPlotPoints != NULL )
	{
		PlDestroyLinkedList( self->brushPlotPoints );
		self->brushPlotPoints = NULL;
	}
}

void ape_editor_set_active_instance( ApeEditorState *instance )
{
	editorInstance = instance;
}

ApeEditorState *ape_editor_get_active_instance( void )
{
	return editorInstance;
}

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

static ApeViewport *selectionViewport;
static PLHashTable *selectionObjectTable;

ApeViewport *get_selection_viewport_( void )
{
	return selectionViewport;
}

/////////////////////////////////////////////////////////////////////////////////////

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
	selectionObjectTable = PlCreateHashTable();
	if ( selectionObjectTable == NULL )
	{
		ape_error_( true, "Failed to create selection object hash table: %s\n", PlGetError() );
	}

	selectionViewport = ape_viewport_create( 0, 0, 640, 480, NULL );
	if ( selectionViewport == NULL )
	{
		ape_error_( true, "Failed to create selection viewport!\n" );
	}

	planeMaterial = ss_arl_material_cache( "materials/editor/plane.mat.n", APE_CACHE_EDITOR, true, false );
}

void ape_shutdown_editor_( void )
{
	PlDestroyHashTable( selectionObjectTable );

	ape_viewport_destroy( selectionViewport );

	grid_shutdown_();
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_toggle_grid_command_( unsigned int, char ** );

void ape_register_editor_console_variables_( void )
{
	PlRegisterConsoleCommand( "editor", "Toggle main editor functionality.", 0, toggle_editor_command );
	PlRegisterConsoleCommand( "toggle_grid", "Toggle the editing grid.", 0, ape_toggle_grid_command_ );

	PlRegisterConsoleCommand( "editor_save_world", "Save the current level with the specified name.", 1, save_world_command );
	PlRegisterConsoleCommand( "editor_create_world", "Create a new world instance.", 0, create_world_command );
}

void ape_editor_pre_render_scene_( const ApeCamera *camera )
{
	if ( !ape_is_editor_active() )
	{
		return;
	}

	ApeEditorState *instance = ape_editor_get_active_instance();
	if ( instance == NULL )
	{
		return;
	}


	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
	PLLinkedListNode *node = PlGetFirstNode( instance->brushPlotPoints );
	while ( node != NULL )
	{
		const PLVector3 *p = PlGetLinkedListNodeUserData( node );
		assert( p != NULL );

		PlgImmPushVertex( p->x, p->y, p->z );
		PlgImmColour( 255, 255, 0, 255 );

		node = PlGetNextLinkedListNode( node );
	}

	PlgGenerateTextureCoordinates( mesh->vertices, mesh->num_verts, pl_vecOrigin2, PL_VECTOR2( 0.5f, 0.5f ) );

	ss_arl_material_draw( planeMaterial, mesh, NULL, 0 );

	PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
	node = PlGetFirstNode( instance->brushPlotPoints );
	while ( node != NULL )
	{
		const PLVector3 *p = PlGetLinkedListNodeUserData( node );
		assert( p != NULL );

		PLCollisionAABB bounds = {
		        .origin = *p,
		        .mins = {-0.1f, -0.1f, -0.1f},
		        .maxs = { 0.1f, 0.1f,  0.1f },
		};
		PlgDrawBoundingVolume( &bounds, &PL_COLOUR_PURPLE );

		node = PlGetNextLinkedListNode( node );
	}
}

void ape_editor_draw_gui_( const ApeViewport *viewport )
{
	if ( !ape_is_editor_active() || editorInstance == NULL )
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

	GuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_SMALL );

	float dw, dh;
	gui_font_get_string_pixel_size( font, 1.0f, label, strlen( label ), &dw, &dh );
	gui_font_draw_string( font, ( float ) viewport->width - ( dw + dh ), ( float ) viewport->height - ( ( dh * 2.0f ) - 2.0f ), NULL, NULL, 1.0f, &PL_COLOUR_WHITE, label, strlen( label ), true );

	unsigned int num = 1;
	PLLinkedListNode *node = PlGetFirstNode( editorInstance->brushPlotPoints );
	while ( node != NULL )
	{
		PLVector3 *p = PlGetLinkedListNodeUserData( node );
		assert( p != NULL );

		PLMatrix4 m = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
		PLVector2 screenPos = PlConvertWorldToScreen( p, &m, viewport->width, viewport->height, viewport->x, viewport->y, true );

		char msg[ 8 ];
		snprintf( msg, sizeof( msg ), "%u", num++ );
		gui_font_draw_string( font, screenPos.x, screenPos.y, NULL, NULL, 1.0f, &PL_COLOUR_WHITE, msg, strlen( msg ), true );

		node = PlGetNextLinkedListNode( node );
	}

	gui_font_display( font );

	if ( camera->mode != APE_CAMERA_MODE_INVALID && camera->mode != APE_CAMERA_MODE_PERSPECTIVE )
	{
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

		float z = viewport->zoom;
		int zoom = round( z ) / 2;
		if ( zoom <= 0 )
		{
			zoom = 1;
		}

		int x = 500 + sin( zoom * 2 ) * 100;
		int y = 200 + cos( zoom * 2 ) * 100;

		PlMatrixMode( PL_MODELVIEW_MATRIX );//TODO: should probably be view matrix...
		PlPushMatrix();
		PlLoadIdentityMatrix();

		PlgDrawGrid( 0, 0, viewport->width, viewport->height, ( editorInstance->gridScale / 2 ) * zoom, &( PLColour ){ 0, 0, 100, 255 } );
		PlgDrawGrid( 0, 0, viewport->width, viewport->height, editorInstance->gridScale * zoom, &( PLColour ){ 0, 0, 255, 255 } );

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

/////////////////////////////////////////////////////////////////////////////////////
// Brush Plotting
/////////////////////////////////////////////////////////////////////////////////////

static bool validate_plotted_plane( ApeEditorState *state )
{
	// this determines that the plane is convex, hopefully

	unsigned int numVertices;
	PLVector3 **vertices = ( PLVector3 ** ) PlArrayFromLinkedList( state->brushPlotPoints, &numVertices );
	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		PLVector2 a;
		a.x = vertices[ ( i + 2 ) % numVertices ]->x - vertices[ ( i + 1 ) % numVertices ]->x;
		a.y = vertices[ ( i + 2 ) % numVertices ]->y - vertices[ ( i + 1 ) % numVertices ]->y;

		PLVector2 b;
		b.x = vertices[ i ]->x - vertices[ ( i + 1 ) % numVertices ]->x;
		b.y = vertices[ i ]->y - vertices[ ( i + 1 ) % numVertices ]->y;

		float cp = a.x * b.y - a.y * b.x;
		if ( i == 0 )
		{
			sign = cp > 0.0f;
		}
		else if ( sign != ( cp > 0 ) )
		{
			return false;
		}
	}

	PL_DELETE( vertices );

	return true;
}

bool ape_editor_plot_point( ApeEditorState *state )
{
	PLVector3 cursor;
	if ( ape_grid_get_cursor_position( &cursor ) == NULL )
	{
		return false;
	}

	PLVector3 *p = PL_NEW( PLVector3 );
	PLLinkedListNode *node = PlInsertLinkedListNode( state->brushPlotPoints, p );
	*p = cursor;

	// validate and then if this fails, remove the last element
	if ( !validate_plotted_plane( state ) )
	{
		PlDestroyLinkedListNode( node );
		PL_DELETE( p );
		return false;
	}

	return true;
}

static void destroy_plot_point( void *user )
{
	PL_DELETE( ( PLVector3 * ) user );
}

void ape_editor_clear_plot_points( ApeEditorState *state )
{
	PlDestroyLinkedListNodesEx( state->brushPlotPoints, destroy_plot_point );
}
