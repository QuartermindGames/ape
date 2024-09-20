// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Primary code for dealing with editor functionality.

#include "plcore/pl_hashtable.h"
#include "ape_private.h"
#include "common_project.h"
#include "editor.h"
#include "client/renderer/renderer.h"
#include "game/game_public.h"
#include "world/world.h"
#include "yin/gui_public.h"

static AcmBranch *editorConfigRoot;

static ApeMaterial *nodeIcons[ APE_WORLD_MAX_NODE_TYPES ];

static void cache_node_icons( void )
{
	nodeIcons[ APE_WORLD_NODE_TYPE_EMPTY ]  = ape_material_cache( "materials/editor/icon_node.mat.n", APE_CACHE_GROUP_EDITOR, true, false );
	nodeIcons[ APE_WORLD_NODE_TYPE_ROOM ]   = ape_material_cache( "materials/editor/icon_room.mat.n", APE_CACHE_GROUP_EDITOR, true, false );
	nodeIcons[ APE_WORLD_NODE_TYPE_BRUSH ]  = ape_material_cache( "materials/editor/icon_brush.mat.n", APE_CACHE_GROUP_EDITOR, true, false );
	nodeIcons[ APE_WORLD_NODE_TYPE_LIGHT ]  = ape_material_cache( "materials/editor/icon_light.mat.n", APE_CACHE_GROUP_EDITOR, true, false );
	nodeIcons[ APE_WORLD_NODE_TYPE_CAMERA ] = ape_material_cache( "materials/editor/icon_camera.mat.n", APE_CACHE_GROUP_EDITOR, true, false );
	nodeIcons[ APE_WORLD_NODE_TYPE_ENTITY ] = ape_material_cache( "materials/editor/icon_entity.mat.n", APE_CACHE_GROUP_EDITOR, true, false );

	// root is weird; as in theory, you shouldn't see it...
	nodeIcons[ APE_WORLD_NODE_TYPE_ROOT ] = nodeIcons[ APE_WORLD_NODE_TYPE_EMPTY ];
}

static void release_node_icons( void )
{
	for ( uint i = 0; i < APE_WORLD_MAX_NODE_TYPES; ++i )
	{
		if ( nodeIcons[ i ] == NULL )
		{
			continue;
		}

		ape_material_release( nodeIcons[ i ] );
		nodeIcons[ i ] = nullptr;
	}
}

static ApeMaterial *planeMaterial;

static PLVectorArray *materialsArray;// ApeMaterial

static void cache_material_preview_callback( const char *path, void *user )
{
	ApeMaterial *material = ape_material_cache( path, APE_CACHE_GROUP_EDITOR, false, true );
	if ( material == NULL )
	{
		return;
	}

	PlPushBackVectorArrayElement( materialsArray, material );
}

static int compare_materials( const void *a, const void *b )
{
	const char *strA = ape_material_get_path( ( ApeMaterial * ) a );
	const char *strB = ape_material_get_path( ( ApeMaterial * ) b );
	return strcmp( strA, strB );
}

static void cache_preview_materials( void )
{
	ape_print_( "Attempting to cache preview materials...\n" );

	materialsArray = PlCreateVectorArray( 256 );
	if ( materialsArray == NULL )
	{
		ape_error_( true, "Failed to create material array for editor: %s\n", PlGetError() );
	}

	// Cache all the materials in a preview state
	AcmBranch *child = acm_branch_get_child_by_name( editorConfigRoot, "materialPaths" );
	if ( child == nullptr )
	{
		ape_warning_( "No material paths specified for editor!\n" );
		return;
	}

	child = acm_branch_get_first_child( child );
	while ( child != nullptr )
	{
		PLPath buf;
		acm_branch_get_string( child, buf, sizeof( buf ) );
		PlScanDirectory( buf, "n", cache_material_preview_callback, true, NULL );
		child = acm_get_next_child( child );
	}

	uint          numMaterials;
	ApeMaterial **materials = ( ApeMaterial ** ) PlGetVectorArrayDataEx( materialsArray, &numMaterials );
	PRINT( "Found %u materials\n", numMaterials );

	qsort( materials, numMaterials, sizeof( ApeMaterial * ), compare_materials );
}

static void release_preview_materials( void )
{
	uint          numMaterials;
	ApeMaterial **materials = ( ApeMaterial ** ) PlGetVectorArrayDataEx( materialsArray, &numMaterials );
	for ( uint i = 0; i < numMaterials; ++i )
	{
		ape_material_release( materials[ i ] );
	}

	PlDestroyVectorArray( materialsArray );
}

static const char *edit_mode_descriptor( ApeEditorGeometryMode mode )
{
	switch ( mode )
	{
		default: return "unknown";
		case APE_EDITOR_GEOMETRY_MODE_PLOT: return "poly";
		case APE_EDITOR_GEOMETRY_MODE_VERTEX: return "vertex";
		case APE_EDITOR_GEOMETRY_MODE_FACE: return "face";
		case APE_EDITOR_GEOMETRY_MODE_EDGE: return "edge";
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM: return "transform";
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Editor Instance Management
/////////////////////////////////////////////////////////////////////////////////////

extern const ApeEditorModeInterface  ape_editorVectorModeInterface_;
static const ApeEditorModeInterface *editorModeInterfaces[ APE_EDITOR_MAX_MODES ] = {
        [APE_EDITOR_MODE_VECTOR] = &ape_editorVectorModeInterface_,
};

void ape_grid_setup_( ApeEditorGrid *self );
void ape_grid_cleanup_( ApeEditorGrid *self );

static PLLinkedList      *editorInstanceList;
static ApeEditorInstance *editorInstance;

ApeEditorInstance *ape_editor_instance_create_( ApeEditorMode mode )
{
	ApeEditorInstance *instance = PL_NEW( ApeEditorInstance );

	if ( ape_editor_instance_setup( instance, mode ) == nullptr )
	{
		PL_DELETEN( instance );
	}

	if ( instance != NULL )
	{
		instance->managed = true;
	}

	return instance;
}

ApeEditorInstance *ape_editor_instance_setup( ApeEditorInstance *self, ApeEditorMode mode )
{
	PL_ZERO( self, sizeof( ApeEditorInstance ) );

	self->mode         = mode;
	self->geometryMode = APE_EDITOR_GEOMETRY_MODE_PLOT;

	self->brushPlotPoints = PlCreateLinkedList();
	if ( self->brushPlotPoints == NULL )
	{
		ape_warning_( "Failed to create brush plot points list: %s\n", PlGetError() );
		return nullptr;
	}

	ape_grid_setup_( &self->grid );

	if ( editorModeInterfaces[ self->mode ] != nullptr && editorModeInterfaces[ self->mode ]->setup != nullptr )
	{
		if ( !editorModeInterfaces[ self->mode ]->setup( self ) )
		{
			return nullptr;
		}
	}

	if ( editorInstanceList == nullptr )
	{
		editorInstanceList = PlCreateLinkedList();
		if ( editorInstanceList == nullptr )
		{
			ape_error_( true, "Failed to create editor instance list: %s\n", PlGetError() );
		}

		cache_preview_materials();
		cache_node_icons();
	}

	self->listNode = PlInsertLinkedListNode( editorInstanceList, self );

	return self;
}

void ape_editor_instance_cleanup( ApeEditorInstance *self )
{
	if ( editorModeInterfaces[ self->mode ]->cleanup != nullptr )
	{
		editorModeInterfaces[ self->mode ]->cleanup( self );
	}

	if ( self->brushPlotPoints != NULL )
	{
		PlDestroyLinkedList( self->brushPlotPoints );
		self->brushPlotPoints = nullptr;
	}

	ape_grid_cleanup_( &self->grid );

	PlDestroyLinkedListNode( self->listNode );
	self->listNode = nullptr;

	if ( editorInstanceList != nullptr && PlIsLinkedListEmpty( editorInstanceList ) )
	{
		// no instances left, completely clean up!

		PlDestroyLinkedList( editorInstanceList );
		editorInstanceList = nullptr;

		release_preview_materials();
		release_node_icons();
	}

	if ( editorInstance == self )
	{
		ape_editor_set_active_instance( nullptr );
	}

	// the engine needs to destroy managed ones itself
	if ( self->managed )
	{
		PL_DELETE( self );
	}
}

void ape_editor_set_active_instance( ApeEditorInstance *instance )
{
	editorInstance = instance;
}

ApeEditorInstance *ape_editor_get_active_instance( void )
{
	return editorInstance;
}

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

static ApeViewport *selectionViewport;
static PLHashTable *selectionObjectTable;

ApeViewport *ape_editor_get_selection_viewport_( void )
{
	return selectionViewport;
}

/////////////////////////////////////////////////////////////////////////////////////

static void editor_command( uint argc, char **argv )
{
	ApeEditorMode mode = APE_EDITOR_MODE_INVALID;

	const char *cmd = argv[ 1 ];
	if ( strcmp( cmd, "vector" ) == 0 )
	{
		mode = APE_EDITOR_MODE_VECTOR;
	}
	else if ( strcmp( cmd, "world" ) == 0 )
	{
		mode = APE_EDITOR_MODE_WORLD;
	}
	else if ( strcmp( cmd, "model" ) == 0 )
	{
		mode = APE_EDITOR_MODE_MODEL;
	}
	else
	{
		ape_warning_( "Unknown editor mode specified (%s)!\n", cmd );
		return;
	}

	ApeEditorInstance *instance = ape_editor_instance_create_( mode );
	if ( instance == nullptr )
	{
		return;
	}

	ape_editor_set_active_instance( instance );
}

static void close_editor_command( uint, char ** )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		ape_warning_( "No active instance to close!\n" );
		return;
	}

	if ( !instance->managed )
	{
		ape_warning_( "Current context isn't managed by the engine!\n" );
		return;
	}

	ape_editor_instance_cleanup( instance );
}

static void save_command( uint, char **argv )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		ape_warning_( "No active instance to close!\n" );
		return;
	}

	const char *cmd = argv[ 1 ];
	if ( editorModeInterfaces[ instance->mode ] == nullptr || editorModeInterfaces[ instance->mode ]->save == nullptr )
	{
		ape_warning_( "No save function available for this editor mode!\n" );
		return;
	}

	editorModeInterfaces[ instance->mode ]->save( instance, cmd );
}

static void load_command( uint, char **argv )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		ape_warning_( "No active instance to close!\n" );
		return;
	}

	const char *cmd = argv[ 1 ];
	if ( editorModeInterfaces[ instance->mode ] == nullptr || editorModeInterfaces[ instance->mode ]->load == nullptr )
	{
		ape_warning_( "No load function available for this editor mode!\n" );
		return;
	}

	editorModeInterfaces[ instance->mode ]->load( instance, cmd );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_editor_( void )
{
	AcmBranch *root = com_project_get_config();
	assert( root != NULL );
	editorConfigRoot = acm_branch_get_child_by_name( root, "editor" );
	if ( editorConfigRoot == nullptr )
	{
		editorConfigRoot = acm_branch_push_back_object( root, "editor" );
	}

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

	planeMaterial = ape_material_cache( "materials/editor/plane.mat.n", APE_CACHE_GROUP_EDITOR, true, false );

	for ( uint i = 0; i < APE_EDITOR_MAX_MODES; ++i )
	{
		if ( editorModeInterfaces[ i ] == nullptr || editorModeInterfaces[ i ]->initialize == nullptr )
		{
			continue;
		}

		editorModeInterfaces[ i ]->initialize();
	}
}

void ape_shutdown_editor_( void )
{
	for ( uint i = 0; i < APE_EDITOR_MAX_MODES; ++i )
	{
		if ( editorModeInterfaces[ i ] == nullptr || editorModeInterfaces[ i ]->shutdown == nullptr )
		{
			continue;
		}

		editorModeInterfaces[ i ]->shutdown();
	}

	PlDestroyHashTable( selectionObjectTable );

	ape_viewport_destroy( selectionViewport );
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_grid_toggle_command_( uint, char ** );

void ape_editor_register_console_( void )
{
	PlRegisterConsoleCommand( "editor", "Launch an editor instance.", 1, editor_command );
	PlRegisterConsoleCommand( "editor_close", "Close the current instance.", 0, close_editor_command );
	PlRegisterConsoleCommand( "editor_toggle_grid", "Toggle the editing grid.", 0, ape_grid_toggle_command_ );
	PlRegisterConsoleCommand( "editor_save", "Save the current instance.", 1, save_command );
	PlRegisterConsoleCommand( "editor_load", "Load for the current instance.", 1, load_command );
}

static void pre_render_nodes( ApeCamera *camera, const ApeWorld *world, const ApeWorldNode *worldNode )
{
	assert( world != NULL && worldNode != NULL );

	// don't draw the sprite for the camera...
	const ApeWorldNode *cameraNode = ( ApeWorldNode * ) camera;
	if ( worldNode == cameraNode )
	{
		return;
	}

	const PLVector3 position = worldNode->position;
	if ( nodeIcons[ worldNode->type ] != NULL )
	{
		static const float size  = 64.0f;
		static const float scale = 0.1f;
		ape_draw_sprite( nodeIcons[ worldNode->type ],
		                 &PL_QUAD( 0.0f, 0.0f, size, size ),
		                 &PL_COLOURF32RGB( 1.0f, 1.0f, 1.0f ),
		                 &PL_VECTOR3( position.x, position.y, position.z ),
		                 &PL_VECTOR3( -( ( size / 2.0f ) * scale ), -( ( size / 2.0f ) * scale ), -( ( size / 2.0f ) * scale ) ),
		                 &PL_VECTOR3( 0.0f, camera->internal->angles.y, 0.0f ),
		                 scale );
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	// this is handled during simulation now via debug draw api
	//PlgDrawBoundingVolume( &worldNode->bounds, &PL_COLOURU8( 255, 0, 255, 255 ) );

	PLLinkedListNode *node = PlGetFirstNode( worldNode->children );
	while ( node != NULL )
	{
		ApeWorldNode *childWorldNode = PlGetLinkedListNodeUserData( node );
		pre_render_nodes( camera, world, childWorldNode );
		node = PlGetNextLinkedListNode( node );
	}
}

static void draw_brush_gui( const ApeViewport *viewport, GuiFont *font )
{
	ApeCamera *camera = viewport->camera;
	assert( camera != NULL );

	uint              num  = 1;
	PLLinkedListNode *node = PlGetFirstNode( editorInstance->brushPlotPoints );
	while ( node != NULL )
	{
		PLVector3 *p = PlGetLinkedListNodeUserData( node );
		assert( p != NULL );

		PLMatrix4 m              = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
		int       viewportSize[] = { viewport->x, viewport->y, viewport->width, viewport->height };
		PLVector2 screenPos      = PlConvertWorldToScreen( p, &m, viewportSize, true );
		if ( screenPos.x == 0.0f && screenPos.y == 0.0f )
		{
			return;
		}

		screenPos.x = roundf( screenPos.x );
		screenPos.y = roundf( screenPos.y );

		char msg[ 16 ];
		snprintf( msg, sizeof( msg ), "%u", num++ );
		gui_font_draw_string( font, screenPos.x, screenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, msg, strlen( msg ), true );

		node = PlGetNextLinkedListNode( node );

		PLVector3 *e;
		if ( node == NULL )
		{
			e = PlGetLinkedListNodeUserData( PlGetFirstNode( editorInstance->brushPlotPoints ) );
		}
		else
		{
			e = PlGetLinkedListNodeUserData( node );
		}
		assert( e != NULL );

		PLVector2 otherScreenPos = PlConvertWorldToScreen( e, &m, viewportSize, true );
		if ( otherScreenPos.x == 0.0f && otherScreenPos.y == 0.0f )
		{
			return;
		}

		// determine the point between the two on the screen
		PLVector2 midpointScreenPos = { ( screenPos.x + otherScreenPos.x ) / 2.0f, ( screenPos.y + otherScreenPos.y ) / 2.0f };
		snprintf( msg, sizeof( msg ), "%f", PlVector3Length( PlSubtractVector3( *e, *p ) ) );
		gui_font_draw_string( font, midpointScreenPos.x, midpointScreenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, msg, strlen( msg ), true );
	}
}

static void pre_render_brush( ApeEditorInstance *instance )
{
	PLGMesh          *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
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

	ape_material_draw( planeMaterial, mesh, nullptr );

	// draw boundary
	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );
	node = PlGetFirstNode( instance->brushPlotPoints );
	while ( true )
	{
		if ( node == NULL )
		{
			break;
		}

		const PLVector3 *p = PlGetLinkedListNodeUserData( node );
		assert( p != NULL );

		PLCollisionAABB bounds = {
		        .origin = *p,
		        .mins   = {-0.1f, -0.1f, -0.1f},
		        .maxs   = {0.1f,  0.1f,  0.1f },
		};
		PlgDrawBoundingVolume( &bounds, &PL_COLOUR_PURPLE );

		const PLVector3 *e;
		node = PlGetNextLinkedListNode( node );
		if ( node != NULL )
		{
			e = PlGetLinkedListNodeUserData( node );
		}
		else
		{
			e = PlGetLinkedListNodeUserData( PlGetFirstNode( instance->brushPlotPoints ) );
		}
		assert( e != NULL );
		PlgDrawSimpleLine( PlMatrix4Identity(), *p, *e, PL_COLOUR_PURPLE );
	}
	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
}

void ape_editor_pre_render_scene_( ApeCamera *camera )
{
	if ( !ape_is_editor_active() )
	{
		return;
	}

	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == NULL )
	{
		return;
	}

	// slow, unoptimised, jelly

	switch ( instance->geometryMode )
	{
		case APE_EDITOR_GEOMETRY_MODE_PLOT:
			pre_render_brush( instance );
			break;
		case APE_EDITOR_GEOMETRY_MODE_FACE: break;
		case APE_EDITOR_GEOMETRY_MODE_EDGE: break;
		case APE_EDITOR_GEOMETRY_MODE_VERTEX:
			break;
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM: break;
		case APE_EDITOR_MAX_GEOMETRY_MODES: break;
	}

	const ApeWorld *world = ape_camera_get_world( camera );
	if ( world != nullptr )
	{
		bool isWireframe = PlgIsGraphicsStateEnabled( PLG_GFX_STATE_WIREFRAME );
		if ( isWireframe )
		{
			PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
		}

		pre_render_nodes( camera, world, &world->base );

		if ( isWireframe )
		{
			PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
		}
	}
}

void ape_editor_draw_gui_( const ApeViewport *viewport )
{
	if ( !ape_is_editor_active() || editorInstance == nullptr )
	{
		return;
	}

	if ( editorModeInterfaces[ editorInstance->mode ] != nullptr && editorModeInterfaces[ editorInstance->mode ]->drawOverlay != nullptr )
	{
		editorModeInterfaces[ editorInstance->mode ]->drawOverlay( editorInstance );
	}

	ApeCamera *camera = viewport->camera;
	if ( camera == nullptr )
	{
		return;
	}

	GuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_SMALL );

	if ( editorInstance->camera != nullptr )
	{
		char label[ 64 ] = {};
		S_STRCAT( label, ape_get_camera_draw_mode_label( camera->drawMode ) );
		S_STRCAT( label, " / " );
		S_STRCAT( label, ape_get_camera_view_mode_label( camera->mode ) );

		float dw, dh;
		gui_font_get_string_pixel_size( font, 1.0f, label, strlen( label ), &dw, &dh );
		gui_font_draw_string( font, ( float ) viewport->width - ( dw + dh ), ( float ) viewport->height - ( ( dh * 2.0f ) - 2.0f ), nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, label, strlen( label ), true );

		// check the camera has a valid room, otherwise display a warning
		ApeRoom *room = ape_camera_get_room( editorInstance->camera );
		if ( room != nullptr )
		{
			char buf[ 256 ];
			snprintf( buf, sizeof( buf ), "Room: %s\nMode: %s\n", room->base.name, edit_mode_descriptor( editorInstance->geometryMode ) );
			gui_font_draw_string( font, 0.0f, 0.0f, &dw, &dh, 1.0f, &PL_COLOUR_WHITE, buf, strlen( buf ), false );
		}
		else
		{
			static const char *warning = "No active room for camera!\n";
			gui_font_draw_string( font, 0.0f, 0.0f, &dw, &dh, 1.0f, &PL_COLOUR_CRIMSON, warning, strlen( warning ), false );
		}

		if ( camera->mode == APE_CAMERA_MODE_PERSPECTIVE )
		{
			PLVector3 pos;
			if ( ape_grid_get_cursor_position( &editorInstance->grid, &pos ) != nullptr )
			{
				// sigh...
				PLMatrix4 view     = camera->internal->internal.view;
				PLMatrix4 proj     = camera->internal->internal.proj;
				PLMatrix4 viewProj = PlMultiplyMatrix4( proj, &view );

				ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

				static constexpr float scale = 16.0f;

				PLVector2 screenPos = PlConvertWorldToScreen( &pos, &viewProj, ( int[] ){ 0, 0, viewport->width, viewport->height }, true );
				PlgDrawLineRectangle( screenPos.x - ( scale / 2.0f ), screenPos.y - ( scale / 2.0f ), scale, scale, PL_COLOUR_WHITE );
			}
		}
	}

	draw_brush_gui( viewport, font );

	gui_font_display( font );

#if 0
	if ( camera->mode != APE_CAMERA_MODE_INVALID && camera->mode != APE_CAMERA_MODE_PERSPECTIVE )
	{
		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

		float z = viewport->zoom;
		float zoom = roundf( z ) / 2;
		if ( zoom <= 0 )
		{
			zoom = 1;
		}

		float x = 500.0f + sinf( zoom * 2 ) * 100.0f;
		float y = 200.0f + cosf( zoom * 2 ) * 100.0f;

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
#endif
}

bool ape_is_editor_active( void )
{
	return ( ape_editor_get_active_instance() != nullptr );
}

ApeMaterial **ape_editor_get_available_materials( uint *numMaterials )
{
	if ( materialsArray == NULL )
	{
		return nullptr;
	}

	return ( ApeMaterial ** ) PlGetVectorArrayDataEx( materialsArray, numMaterials );
}

/////////////////////////////////////////////////////////////////////////////////////
// Brush Plotting
/////////////////////////////////////////////////////////////////////////////////////

//TODO: update to use com_math_is_plane_convex!!
static bool validate_plotted_plane( ApeEditorInstance *state )
{
	// this determines that the plane is convex, hopefully

	uint        numVertices;
	PLVector3 **vertices = ( PLVector3 ** ) PlArrayFromLinkedList( state->brushPlotPoints, &numVertices );
	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( uint i = 0; i < numVertices; ++i )
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

bool ape_editor_plot_point( ApeEditorInstance *state )
{
	PLVector3 cursor;
	if ( ape_grid_get_cursor_position( &state->grid, &cursor ) == NULL )
	{
		return false;
	}

	PLVector3        *p    = PL_NEW( PLVector3 );
	PLLinkedListNode *node = PlInsertLinkedListNode( state->brushPlotPoints, p );
	*p                     = cursor;

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

void ape_editor_clear_plot_points( ApeEditorInstance *state )
{
	PlDestroyLinkedListNodesEx( state->brushPlotPoints, destroy_plot_point );
}
