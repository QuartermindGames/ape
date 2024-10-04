// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Primary code for dealing with editor functionality.

#include "plcore/pl_hashtable.h"

#include "ape_private.h"

#include "common_project.h"
#include "editor.h"
#include "client/renderer/renderer.h"
#include "world/world.h"
#include "yin/gui_public.h"

static AcmBranch *editorConfigRoot;

static bool         showIcons;
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

	self->selectionTable = PlCreateHashTable();
	if ( self->selectionTable == nullptr )
	{
		ape_error_( true, "Failed to create selection table for instance: %s\n", PlGetError() );
	}

	return self;
}

void ape_editor_instance_cleanup( ApeEditorInstance *self )
{
	if ( editorModeInterfaces[ self->mode ] != nullptr && editorModeInterfaces[ self->mode ]->cleanup != nullptr )
	{
		editorModeInterfaces[ self->mode ]->cleanup( self );
	}

	self->numPolygonPoints = 0;

	ape_grid_cleanup_( &self->grid );

	PlDestroyLinkedListNode( self->listNode );
	self->listNode = nullptr;

	PlDestroyHashTable( self->selectionTable );
	self->selectionTable = nullptr;

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

// todo: these should be linked against the active instance
static ApeViewport *selectionViewport;

void ape_grid_draw_selection_( ApeEditorGrid *self );

static void add_brush_faces_to_selection_table( ApeEditorInstance *self, ApeBrush *brush )
{
	for ( uint i = 0; i < brush->numFaces; ++i )
	{
		uint64_t hash = PlGenerateHashFNV1( &brush->faces[ i ], sizeof( ApeBrushFace ) );

		brush->faces[ i ].selectColour.r = ( hash >> 16 ) & 0xFF;
		brush->faces[ i ].selectColour.g = ( hash >> 8 ) & 0xFF;
		brush->faces[ i ].selectColour.b = hash & 0xFF;
		brush->faces[ i ].selectColour.a = 255;

		PlInsertHashTableNode( self->selectionTable, &brush->faces[ i ].selectColour, sizeof( PLColour ), &brush->faces[ i ] );
	}

	ApeWorldNode *node;
	COM_ITERATE_LINKED_LIST( node, brush->base.children, i )
	{
		if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		add_brush_faces_to_selection_table( self, ( ApeBrush * ) node );
	}
}

static void rebuild_selection_object_table( ApeEditorInstance *self )
{
	self->selectedObject = nullptr;
	PlClearHashTable( self->selectionTable );

	ApeCamera *camera = self->camera;
	assert( camera != nullptr );

	ApeRoom *room = ape_camera_get_room( camera );
	assert( room != nullptr );

	switch ( self->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			ApeWorldNode *node;
			COM_ITERATE_LINKED_LIST( node, room->base.children, i )
			{
				if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
				{
					continue;
				}

				add_brush_faces_to_selection_table( self, ( ApeBrush * ) node );
			}
			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
		{
			ApeWorldNode *node;
			COM_ITERATE_LINKED_LIST( node, room->base.children, i )
			{
			}
			break;
		}
	}
}

static void render_brush_faces( ApeEditorInstance *self, ApeWorldNode *root )
{
	ApeWorldNode *node;
	COM_ITERATE_LINKED_LIST( node, root->children, i )
	{
		render_brush_faces( self, node );

		if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		//todo: optimize this... :(
		ApeBrush *brush = ( ApeBrush * ) node;
		for ( uint i = 0; i < brush->numFaces; ++i )
		{
			PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
			for ( uint j = 0; j < brush->faces[ i ].numVertices; ++j )
			{
				const ApeBrushFaceVertex *vertex = brush->faces[ i ].edgeLoop[ j ];
				PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
				PlgImmColour( brush->faces[ i ].selectColour.r, brush->faces[ i ].selectColour.g, brush->faces[ i ].selectColour.b, 255 );
			}

			ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
			assert( material != nullptr );

			ape_material_draw( material, mesh, nullptr );
		}
	}
}

static void render_selection_buffer( ApeEditorInstance *self )
{
	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == nullptr )
	{
		return;
	}

	ApeCamera *camera = self->camera;
	assert( camera != nullptr );

	ApeRoom *room = ape_camera_get_room( camera );
	assert( room != nullptr );

//#define DEBUG_GRID_SELECTION
#if !defined( DEBUG_GRID_SELECTION )
	ApeViewport *selectionViewport = ape_editor_get_selection_viewport_();

	uint sw = viewport->width / 2;
	uint sh = viewport->height / 2;
	ape_viewport_set_size( selectionViewport, sw, sh );
	ape_viewport_make_active( selectionViewport );
	ape_render_target_bind( selectionViewport->renderTarget, PLG_FRAMEBUFFER_DRAW );

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
#endif

	if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_PLOT )
	{
		ape_grid_draw_selection_( &self->grid );
	}
	else if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		render_brush_faces( self, &room->base );
	}

#if !defined( DEBUG_GRID_SELECTION )
	ape_render_target_bind( viewport->renderTarget, PLG_FRAMEBUFFER_DEFAULT );
	ape_viewport_make_active( viewport );
#endif
}

PLColour *ape_editor_get_pixel_under_cursor( PLColour *dst )
{
	ApeViewport    *selectionViewport = ape_editor_get_selection_viewport_();
	PLGFrameBuffer *frameBuffer       = ape_render_target_get_frame_buffer( selectionViewport->renderTarget );
	if ( frameBuffer == nullptr )
	{
		return nullptr;
	}

	size_t    size = frameBuffer->width * frameBuffer->height * 4;
	PLColour *buf  = PL_NEW_( PLColour, size );
	if ( PlgReadFrameBufferRegion( frameBuffer, 0, 0, frameBuffer->width, frameBuffer->height, size, buf ) != nullptr )
	{
		int x, y;
		ape_client_input_get_mouse_position( &x, &y );

		// selection buffer is half of the source
		x /= 2;
		y /= 2;

		if ( x < frameBuffer->width && y < frameBuffer->height )
		{
			*dst = buf[ ( frameBuffer->height - y - 1 ) * frameBuffer->width + x ];
			PL_DELETE( buf );
			return dst;
		}
	}
	else
	{
		ape_warning_( "Failed to read framebuffer: %s\n", PlGetError() );
	}

	PL_DELETE( buf );
	return nullptr;
}

void *ape_editor_get_object_under_cursor( ApeEditorInstance *self )
{
	PLColour pixel;
	if ( ape_editor_get_pixel_under_cursor( &pixel ) == nullptr )
	{
		return nullptr;
	}

	return PlLookupHashTableUserData( self->selectionTable, &pixel, sizeof( PLColour ) );
}

ApeBrushFace *ape_editor_get_selected_brush_face( ApeEditorInstance *self )
{
	if ( self->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return nullptr;
	}

	return self->selectedFace;
}

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

	selectionViewport = ape_viewport_create( 0, 0, 640, 480, NULL );
	if ( selectionViewport == NULL )
	{
		ape_error_( true, "Failed to create selection viewport!\n" );
	}

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

	ape_viewport_destroy( selectionViewport );
}

void ape_editor_set_geometry_mode( ApeEditorInstance *self, ApeEditorGeometryMode geometryMode )
{
	if ( self->geometryMode == geometryMode )
	{
		return;
	}

	self->geometryMode = geometryMode;

	rebuild_selection_object_table( self );
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

	PlRegisterConsoleVariable( "editor.showIcons", "Show icons in the editor mode.", "true", PL_VAR_BOOL, &showIcons, nullptr, true );
}

static void pre_render_nodes( ApeEditorInstance *self, ApeCamera *camera, const ApeWorldNode *worldNode )
{
	assert( worldNode != NULL );

	// don't draw the sprite for the camera...
	const ApeWorldNode *cameraNode = ( ApeWorldNode * ) camera;
	if ( worldNode == cameraNode )
	{
		return;
	}

	if ( showIcons )
	{
		if ( nodeIcons[ worldNode->type ] != NULL )
		{
			static const float size   = 64.0f;
			static const float scale  = 0.1f;
			static const float origin = -( size / 2.0f );

			ape_draw_sprite( nodeIcons[ worldNode->type ],
			                 &PL_QUAD( 0.0f, 0.0f, size, size ),
			                 &PL_COLOURF32RGB( 1.0f, 1.0f, 1.0f ),
			                 &worldNode->position,
			                 &PL_VECTOR3( origin, origin, origin ),
			                 &PL_VECTOR3( 0.0f, camera->internal->angles.y, 0.0f ),
			                 scale );
		}
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	if ( self->currentNode == worldNode )
	{
		// this is handled during simulation now via debug draw api
		PlgDrawBoundingVolume( &worldNode->bounds, &PL_COLOURU8( 0, 255, 0, 255 ) );
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, worldNode->children, i )
	{
		pre_render_nodes( self, camera, child );
	}

	PlPopMatrix();
}

static void draw_brush_gui( const ApeViewport *viewport, GuiFont *font )
{
#if 0//todo
	ApeCamera *camera = viewport->camera;
	assert( camera != NULL );

	for ( uint i = 0, num = 1; i < editorInstance->numPolygonPoints; ++i )
	{
		PLMatrix4 m              = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
		int       viewportSize[] = { viewport->x, viewport->y, viewport->width, viewport->height };
		PLVector2 screenPos      = PlConvertWorldToScreen( &editorInstance->polygonPoints[ i ], &m, viewportSize, true );
		if ( screenPos.x == 0.0f && screenPos.y == 0.0f )
		{
			return;
		}

		screenPos.x = floorf( screenPos.x );
		screenPos.y = floorf( screenPos.y );

		char msg[ 64 ];
		snprintf( msg, sizeof( msg ), "%u", num++ );
		//gui_font_draw_string( font, screenPos.x, screenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, msg, strlen( msg ), true );

		PLVector3 end = ( i + 1 >= editorInstance->numPolygonPoints ) ? editorInstance->polygonPoints[ 0 ] : editorInstance->polygonPoints[ i + 1 ];

		PLVector2 otherScreenPos = PlConvertWorldToScreen( &end, &m, viewportSize, true );
		if ( otherScreenPos.x == 0.0f && otherScreenPos.y == 0.0f )
		{
			return;
		}

		// determine the point between the two on the screen
		PLVector2 midpointScreenPos = { ( screenPos.x + otherScreenPos.x ) / 2.0f, ( screenPos.y + otherScreenPos.y ) / 2.0f };
		snprintf( msg, sizeof( msg ), "%f (%s)", PlVector3Length( PlSubtractVector3( end, editorInstance->polygonPoints[ i ] ) ), PlPrintVector3( &editorInstance->polygonPoints[ i ], PL_VAR_F32 ) );
		gui_font_draw_string( font, midpointScreenPos.x, midpointScreenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, msg, strlen( msg ), true );
	}
#endif
}

static bool validate_convex_polygon( const PLVector2 *vertices, uint numVertices );

static void pre_render_brush( ApeEditorInstance *self )
{
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &self->grid.transform );

	// draw pending polygon
	// and attempt to draw it to the cursor too
	PLVector2 cursor;
	if ( self->numPolygonPoints > 0 )
	{
		if ( ape_grid_get_cursor_position( &self->grid, &cursor ) != nullptr )
		{
			PLColour colour = PL_COLOUR_WHITE;
			if ( self->numPolygonPoints < APE_BRUSH_MAX_FACE_VERTICES )
			{
				self->polygonPoints[ self->numPolygonPoints ] = cursor;
				if ( !validate_convex_polygon( self->polygonPoints, self->numPolygonPoints + 1 ) )
				{
					colour = PL_COLOUR_RED;
				}
			}

			for ( uint i = 0; i < self->numPolygonPoints; ++i )
			{
				PLVector2 *end = ( i + 1 >= self->numPolygonPoints ) ? &cursor : &self->polygonPoints[ i + 1 ];
				PlgDrawSimpleLine(
				        PL_VECTOR3( self->polygonPoints[ i ].x, 0.0f, self->polygonPoints[ i ].y ),
				        PL_VECTOR3( end->x, 0.0f, end->y ),
				        colour );
			}

			PlgDrawSimpleLine(
			        PL_VECTOR3( cursor.x, 0.0f, cursor.y ),
			        PL_VECTOR3( self->polygonPoints[ 0 ].x, 0.0f, self->polygonPoints[ 0 ].y ),
			        colour );
		}
		else
		{
			for ( uint i = 0; i < self->numPolygonPoints; ++i )
			{
				PLVector2 *end = ( i + 1 >= self->numPolygonPoints ) ? &self->polygonPoints[ 0 ] : &self->polygonPoints[ i + 1 ];
				PlgDrawSimpleLine(
				        PL_VECTOR3( self->polygonPoints[ i ].x, 0.0f, self->polygonPoints[ i ].y ),
				        PL_VECTOR3( end->x, 0.0f, end->y ),
				        PL_COLOUR_WHITE );
			}
		}
	}

	PlPopMatrix();

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

	render_selection_buffer( instance );

	switch ( instance->geometryMode )
	{
		case APE_EDITOR_GEOMETRY_MODE_PLOT:
			pre_render_brush( instance );
			break;
		case APE_EDITOR_GEOMETRY_MODE_FACE: break;
		case APE_EDITOR_GEOMETRY_MODE_VERTEX:
			break;
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM: break;
		case APE_EDITOR_MAX_GEOMETRY_MODES: break;
	}

	const ApeRoom *room = ape_camera_get_room( camera );
	if ( room != nullptr )
	{
		bool isWireframe = PlgIsGraphicsStateEnabled( PLG_GFX_STATE_WIREFRAME );
		if ( isWireframe )
		{
			PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
		}

		pre_render_nodes( instance, camera, &room->base );

		if ( isWireframe )
		{
			PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
		}
	}
}

void ape_editor_post_render_scene_( ApeCamera *camera )
{
	ApeEditorInstance *editorInstance = ape_editor_get_active_instance();
	if ( editorInstance == nullptr )
	{
		return;
	}

	if ( editorInstance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		editorInstance->selectedFace = ape_editor_get_object_under_cursor( editorInstance );
		if ( editorInstance->selectedFace != nullptr )
		{
			ApeBrushFace *face = editorInstance->selectedFace;

			PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

			ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
			assert( material != nullptr );

			PLGMesh *mesh = PlgImmBegin( PLG_MESH_LINE_LOOP );
			for ( uint i = 0; i < face->numVertices; ++i )
			{
				const ApeBrushFaceVertex *vertex = face->edgeLoop[ i ];
				PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
				PlgImmColour( 255, 255, 0, 255 );
			}

			ape_material_draw( material, mesh, nullptr );

			PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
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
		char label[ 64 ];
		snprintf( label, sizeof( label ), "%s/%s\n", ape_get_camera_view_mode_label( camera->mode ), ape_get_camera_draw_mode_label( camera->drawMode ) );

		float dw, dh;
		gui_font_get_string_pixel_size( font, 1.0f, label, strlen( label ), &dw, &dh );
		gui_font_draw_string( font, ( float ) viewport->width - ( dw + dh ), ( float ) viewport->height - ( ( dh * 2.0f ) - 2.0f ), nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, label, strlen( label ), true );

		// check the camera has a valid room, otherwise display a warning
		ApeRoom *room = ape_camera_get_room( editorInstance->camera );
		if ( room == nullptr )
		{
			static const char *warning = "No active room for camera!\n";
			gui_font_draw_string( font, 0.0f, 0.0f, &dw, &dh, 1.0f, &PL_COLOUR_CRIMSON, warning, strlen( warning ), false );
		}

		if ( camera->mode == APE_CAMERA_MODE_PERSPECTIVE )
		{
			PLVector2 pos;
			if ( ape_grid_get_cursor_position( &editorInstance->grid, &pos ) != nullptr )
			{
				// sigh...
				PLMatrix4 view     = camera->internal->internal.view;
				PLMatrix4 proj     = camera->internal->internal.proj;
				PLMatrix4 viewProj = PlMultiplyMatrix4( proj, &view );

				ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

				static constexpr float scale = 16.0f;

				PLVector3 worldPos  = ape_grid_transform_point( &editorInstance->grid, &pos );
				PLVector2 screenPos = PlConvertWorldToScreen( &worldPos, &viewProj, ( int[] ){ 0, 0, viewport->width, viewport->height }, true );
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
// Polygon Plotting
// TODO: move into editor_world.c
/////////////////////////////////////////////////////////////////////////////////////

#if 0// concave supporting implementation... doesn't really work :(
typedef struct Segment
{
	PLVector2 start;
	PLVector2 end;
} Segment;

static bool test_intersection( Segment s1, Segment s2 )
{
	float d1, d2, d3, d4;
	float x1 = s1.start.x, y1 = s1.start.y, x2 = s1.end.x, y2 = s1.end.y;
	float x3 = s2.start.x, y3 = s2.start.y, x4 = s2.end.x, y4 = s2.end.y;

	d1 = ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 );
	d2 = ( x4 - x3 ) * ( y2 - y3 ) - ( y4 - y3 ) * ( x2 - x3 );
	d3 = ( x2 - x1 ) * ( y3 - y1 ) - ( y2 - y1 ) * ( x3 - x1 );
	d4 = ( x2 - x1 ) * ( y4 - y1 ) - ( y2 - y1 ) * ( x4 - x1 );

	if ( ( ( d1 > 0 && d2 < 0 ) || ( d1 < 0 && d2 > 0 ) ) &&
	     ( ( d3 > 0 && d4 < 0 ) || ( d3 < 0 && d4 > 0 ) ) )
	{
		return true;
	}

	return false;
}

static bool validate_concave_polygon( const PLVector2 *vertices, uint numVertices )
{
	if ( numVertices < 4 )
	{
		return true;
	}

	Segment segments[ numVertices ];
	for ( uint i = 0; i < numVertices; ++i )
	{
		segments[ i ].start.x = vertices[ i ].x;
		segments[ i ].start.y = vertices[ i ].y;
		segments[ i ].end.x   = vertices[ ( i + 1 ) % numVertices ].x;
		segments[ i ].end.y   = vertices[ ( i + 1 ) % numVertices ].y;
	}

	for ( uint i = 0; i < numVertices; ++i )
	{
		for ( uint j = i + 1; j < numVertices; ++j )
		{
			if ( i != j && test_intersection( segments[ i ], segments[ j ] ) )
			{
				return false;
			}
		}
	}

	return true;
}
#endif

static bool validate_convex_polygon( const PLVector2 *vertices, uint numVertices )
{
	// this determines that the plane is convex, hopefully

	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( uint i = 0; i < numVertices; ++i )
	{
		PLVector2 a;
		a.x = vertices[ ( i + 2 ) % numVertices ].x - vertices[ ( i + 1 ) % numVertices ].x;
		a.y = vertices[ ( i + 2 ) % numVertices ].y - vertices[ ( i + 1 ) % numVertices ].y;

		PLVector2 b;
		b.x = vertices[ i ].x - vertices[ ( i + 1 ) % numVertices ].x;
		b.y = vertices[ i ].y - vertices[ ( i + 1 ) % numVertices ].y;

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

	return true;
}

ApeBrush *ape_editor_brush_from_polygon( ApeEditorInstance *self )
{
	if ( self->numPolygonPoints < 3 )
	{
		ape_warning_( "Not enough points to create a brush.\n" );
		return nullptr;
	}

	ApeCamera *camera = self->camera;
	assert( camera != nullptr );

	ApeRoom *room = ape_camera_get_room( camera );
	if ( room == nullptr )
	{
		ape_warning_( "No valid room for brush!\n" );
		return nullptr;
	}

	ApeBrush *brush = ape_brush_create( APE_WORLD_NODE( room ), nullptr, &PL_VECTOR3( 0.0f, 0.0f, 0.0f ), &PL_VECTOR3( 0.0f, 0.0f, 0.0f ) );
	if ( brush == nullptr )
	{
		return nullptr;
	}

	// determine the orientation of the grid
	PLVector3 dir;
	PlExtractMatrix4Directions( &self->grid.transform, nullptr, &dir, nullptr );

	// because the grid operates in 2D space, we need to transform all the vertices into 3D space
	// and use this time to determine the order too...so we can reverse for edge loop if needed
	float      signedArea = 0.0f;
	PLVector3 *vertices   = PL_NEW_( PLVector3, self->numPolygonPoints );
	for ( uint i = 0; i < self->numPolygonPoints; ++i )
	{
		// determine order
		uint next = ( i + 1 ) % self->numPolygonPoints;
		signedArea += ( self->polygonPoints[ i ].x * self->polygonPoints[ next ].y - self->polygonPoints[ next ].x * self->polygonPoints[ i ].y );

		// now transform it into 3D space
		vertices[ i ] = PlTransformVector3( &PL_VECTOR3( self->polygonPoints[ i ].x, 0.0f, self->polygonPoints[ i ].y ), &self->grid.transform );
	}

	if ( !ape_brush_build_from_polygon_( brush, vertices, self->numPolygonPoints, dir, self->grid.scale / 2.0f, signedArea ) )
	{
		ape_warning_( "Failed to create brush from polygon!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( brush ) );
		brush = nullptr;
	}
	else
	{
		// make it selected
		self->currentNode = &brush->base;
	}

	PL_DELETE( vertices );

	self->numPolygonPoints = 0;

	return brush;
}

void ape_editor_remove_polygon_point( ApeEditorInstance *self )
{
	if ( self->numPolygonPoints == 0 )
	{
		return;
	}

	self->numPolygonPoints--;
}

bool ape_editor_add_polygon_point( ApeEditorInstance *self )
{
	if ( self->numPolygonPoints >= APE_BRUSH_MAX_FACE_VERTICES )
	{
		ape_warning_( "Hit polygon vertex limit (%u >= %u)!\n", self->numPolygonPoints, APE_BRUSH_MAX_FACE_VERTICES );
		return false;
	}

	PLVector2 cursor;
	if ( ape_grid_get_cursor_position( &self->grid, &cursor ) == NULL )
	{
		return false;
	}

	if ( self->numPolygonPoints > 0 )
	{
		const PLVector2 *start = &self->polygonPoints[ 0 ];
		if ( PlCompareVector2( start, &cursor ) )
		{
			ape_editor_brush_from_polygon( self );
			return true;
		}
	}

	self->polygonPoints[ self->numPolygonPoints++ ] = cursor;

	// validate and then if this fails, remove the last element
	if ( !validate_convex_polygon( self->polygonPoints, self->numPolygonPoints ) )
	{
		self->numPolygonPoints--;
		return false;
	}

	return true;
}

void ape_editor_clear_plot_points( ApeEditorInstance *instance )
{
	instance->numPolygonPoints = 0;
}
