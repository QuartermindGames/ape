// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: For handling selection buffer.

#include "plcore/pl_hashtable.h"
#include "plcore/pl_linkedlist.h"

#include "yin/core_camera.h"
#include "yin/core_renderer.h"

#include "editor.h"
#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

static void render_transform_widget( ApeEditorInstance *instance );

static constexpr float SELECTION_OBJECT_SIZE = 8.0f;
static constexpr float SELECTION_VERTEX_SIZE = 2.0f;

// todo: these should be linked against the active instance
static ApeViewport *selectionViewport;

void ape_grid_draw_selection_( ApeEditorGrid *self );

void ape_editor_selection_initialize_()
{
	selectionViewport = ape_viewport_create( 0, 0, 640, 480, NULL, false );
	if ( selectionViewport == NULL )
	{
		ape_error_( true, "Failed to create selection viewport!\n" );
	}
}

void ape_editor_selection_shutdown_()
{
	ape_viewport_destroy( selectionViewport );
}

static PLColour encode_hash_to_colour( uint64_t hash )
{
	return PL_COLOURU8( ( hash >> 16 ) & 0xFF, ( hash >> 8 ) & 0xFF, hash & 0xFF, 255 );
}

static void add_node_to_face_selection( ApeEditorInstance *self, ApeWorldNode *node, ApeEditorGeometryMode mode )
{
	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		ApeBrush *brush = ( ApeBrush * ) node;
		if ( mode == APE_EDITOR_GEOMETRY_MODE_FACE )
		{
			for ( unsigned int i = 0; i < brush->numFaces; ++i )
			{
				uint64_t hash                  = PlGenerateHashFNV1( &brush->faces[ i ], sizeof( ApeBrushFace ) );
				brush->faces[ i ].selectColour = encode_hash_to_colour( hash );
				PlInsertHashTableNode( self->selectionTable, &brush->faces[ i ].selectColour, sizeof( PLColour ), &brush->faces[ i ] );
			}
		}
		else if ( mode == APE_EDITOR_GEOMETRY_MODE_VERTEX )
		{
			if ( brush->numVertices != brush->numVertexSelectColours )
			{
				brush->vertexSelectColours    = PL_REALLOCA( brush->vertexSelectColours, sizeof( PLColour ) * brush->numVertices );
				brush->numVertexSelectColours = brush->numVertices;
			}
			for ( unsigned int i = 0; i < brush->numVertices; ++i )
			{
				brush->vertexSelectColours[ i ] = encode_hash_to_colour( ( uintptr_t ) &brush->vertices[ i ] );
				PlInsertHashTableNode( self->selectionTable, &brush->vertexSelectColours[ i ], sizeof( PLColour ), &brush->vertices[ i ] );
			}
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		add_node_to_face_selection( self, child, mode );
	}
}

static void add_node_to_transform_selection( ApeEditorInstance *self, ApeWorldNode *node )
{
	uint64_t hash      = PlGenerateHashFNV1( node, sizeof( ApeWorldNode ) );
	node->selectColour = encode_hash_to_colour( hash );
	PlInsertHashTableNode( self->selectionTable, &node->selectColour, sizeof( PLColour ), node );

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		add_node_to_transform_selection( self, child );
	}
}

void ape_editor_selection_rebuild_( ApeEditorInstance *self )
{
	// clear all the selection lists
	PlDestroyLinkedListNodes( self->selectedObjects );
	PlClearHashTable( self->selectionTable );
	self->hoverSelection = nullptr;

	ApeCamera *camera = self->camera;
	assert( camera != nullptr );

	ApeRoom *room = ape_camera_get_room( camera );
	assert( room != nullptr );

	switch ( self->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_VERTEX:
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			add_node_to_face_selection( self, &room->base, self->geometryMode );
			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
		{
			add_node_to_transform_selection( self, &room->base );
			break;
		}
	}
}

static void render_brush_face_selection( const ApeBrush *brush )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	//todo: optimize this... :(
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
		for ( unsigned int j = 0; j < brush->faces[ i ].numVertices; ++j )
		{
			const ApeBrushFaceVertex *vertex = brush->faces[ i ].edgeLoop[ j ];
			PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
			PlgColour4bv( mesh, &brush->faces[ i ].selectColour );
		}

		ape_material_draw( material, mesh, nullptr );
	}
}

static void render_brush_selection( const ApeBrush *brush )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	//todo: optimize this... :(
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
		for ( unsigned int j = 0; j < brush->faces[ i ].numVertices; ++j )
		{
			const ApeBrushFaceVertex *vertex = brush->faces[ i ].edgeLoop[ j ];
			PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
			PlgColour4bv( mesh, &brush->base.selectColour );
		}

		ape_material_draw( material, mesh, nullptr );
	}
}

static void draw_selection_cube( const PLVector3 *position, const PLColour *colour, float scale, bool wireframe )
{
	if ( wireframe )
	{
		ape_draw_debug_aabb( &PL_COLLISION_AABB( *position, PL_VECTOR3( -scale, -scale, -scale ), PL_VECTOR3( scale, scale, scale ) ), *colour );
		return;
	}

	static constexpr float CUBE_VERTICES[ 8 ][ 3 ] = {
	        {-1.0f, -1.0f, -1.0f},
	        {1.0f,  -1.0f, -1.0f},
	        {1.0f,  1.0f,  -1.0f},
	        {-1.0f, 1.0f,  -1.0f},
	        {-1.0f, -1.0f, 1.0f },
	        {1.0f,  -1.0f, 1.0f },
	        {1.0f,  1.0f,  1.0f },
	        {-1.0f, 1.0f,  1.0f }
    };
	static constexpr unsigned int CUBE_INDICES[ 12 ][ 3 ] = {
	        {2, 1, 0},
	        {3, 2, 0},
	        {4, 5, 6},
	        {4, 6, 7},
	        {7, 3, 0},
	        {4, 7, 0},
	        {1, 2, 6},
	        {1, 6, 5},
	        {6, 2, 3},
	        {7, 6, 3},
	        {0, 1, 5},
	        {0, 5, 4}
    };

	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLES );
	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( CUBE_INDICES ); ++i )
	{
		for ( unsigned int j = 0; j < 3; ++j )
		{
			PlgImmPushVertex( position->x + CUBE_VERTICES[ CUBE_INDICES[ i ][ j ] ][ 0 ] * scale,
			                  position->y + CUBE_VERTICES[ CUBE_INDICES[ i ][ j ] ][ 1 ] * scale,
			                  position->z + CUBE_VERTICES[ CUBE_INDICES[ i ][ j ] ][ 2 ] * scale );
			PlgColour4bv( mesh, colour );
		}
	}

	ape_material_draw( material, mesh, nullptr );
}

/**
 * Draw objects into the selection buffer.
 *
 * @param self Editor instance.
 */
void ape_editor_selection_render_( ApeEditorInstance *self )
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

	if ( !ape_config_.renderer.showSelectionBuffer )
	{
		ApeViewport *selectionViewport = ape_editor_selection_get_viewport_();

		unsigned int sw = viewport->width / 2;
		unsigned int sh = viewport->height / 2;
		ape_viewport_set_size( selectionViewport, sw, sh );
		ape_viewport_make_active( selectionViewport );
		ape_render_target_bind( selectionViewport->renderTarget, PLG_FRAMEBUFFER_DRAW );

		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
	}

	if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_PLOT )
	{
		ape_grid_draw_selection_( &self->grid );
	}
	else
	{
		unsigned int   numVisibleNodes;
		ApeWorldNode **visibleNodes = ape_camera_get_visible_nodes_( camera, &numVisibleNodes );

		for ( unsigned int i = 0; i < numVisibleNodes; ++i )
		{
			const ApeWorldNode *node = visibleNodes[ i ];

			if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
			{
				switch ( node->type )
				{
					default:
						break;
					case APE_WORLD_NODE_TYPE_BRUSH:
						render_brush_selection( ( ApeBrush * ) node );
						break;
					case APE_WORLD_NODE_TYPE_ENTITY:
					case APE_WORLD_NODE_TYPE_LIGHT:
					case APE_WORLD_NODE_TYPE_CAMERA:
						draw_selection_cube( &node->position, &node->selectColour, 8.0f, false );
						break;
				}
			}
			else if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE && node->type == APE_WORLD_NODE_TYPE_BRUSH )
			{
				render_brush_face_selection( ( ApeBrush * ) node );
			}
			else if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_VERTEX && node->type == APE_WORLD_NODE_TYPE_BRUSH )
			{
				ApeBrush *brush = ( ApeBrush * ) node;
				for ( unsigned int j = 0; j < brush->numVertices; ++j )
				{
					draw_selection_cube( &brush->vertices[ j ], &brush->vertexSelectColours[ j ], SELECTION_VERTEX_SIZE, false );
				}
			}
		}
	}

	if ( !ape_config_.renderer.showSelectionBuffer )
	{
		ape_render_target_bind( viewport->renderTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_viewport_make_active( viewport );
	}
}

static void render_selected_faces( ApeEditorInstance *self )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_LINES );
	PlgImmSetPrimitiveScale( 2.0f );

	ApeBrushFace *face;
	COM_ITERATE_LINKED_LIST( face, self->selectedObjects, i )
	{
		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			const ApeBrushFaceVertex *v0 = face->edgeLoop[ j ];
			const ApeBrushFaceVertex *v1 = ( j + 1 ) < face->numVertices ? face->edgeLoop[ j + 1 ] : face->edgeLoop[ 0 ];
			PlgImmPushVertex( v0->position->x, v0->position->y, v0->position->z );
			PlgImmColour( 0, 0, 255, 255 );
			PlgImmPushVertex( v1->position->x, v1->position->y, v1->position->z );
			PlgImmColour( 0, 0, 255, 255 );
		}
	}

	//todo: unify this somewhere and only fetch if cursor has moved
	self->hoverSelection = ape_editor_get_object_under_cursor( self );
	if ( self->hoverSelection != nullptr )
	{
		face = self->hoverSelection;
		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			const ApeBrushFaceVertex *v0 = face->edgeLoop[ i ];
			const ApeBrushFaceVertex *v1 = ( i + 1 ) < face->numVertices ? face->edgeLoop[ i + 1 ] : face->edgeLoop[ 0 ];
			PlgImmPushVertex( v0->position->x, v0->position->y, v0->position->z );
			PlgImmColour( 255, 255, 0, 255 );
			PlgImmPushVertex( v1->position->x, v1->position->y, v1->position->z );
			PlgImmColour( 255, 255, 0, 255 );
		}
	}

	ape_material_draw( material, mesh, nullptr );
}

static void render_wireframe_brush( PLGMesh *lineMesh, const ApeBrush *brush, const PLColour *colour )
{
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		const ApeBrushFace *face = &brush->faces[ i ];
		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			const ApeBrushFaceVertex *v0 = face->edgeLoop[ j ];
			const ApeBrushFaceVertex *v1 = ( j + 1 ) < face->numVertices ? face->edgeLoop[ j + 1 ] : face->edgeLoop[ 0 ];
			PlgImmPushVertex( v0->position->x, v0->position->y, v0->position->z );
			PlgColour4bv( lineMesh, colour );
			PlgImmPushVertex( v1->position->x, v1->position->y, v1->position->z );
			PlgColour4bv( lineMesh, colour );
		}
	}
}

static void render_selected_wireframe( ApeWorldNode *node, const PLColour *colour, bool selected )
{
	draw_selection_cube( &node->position, colour, SELECTION_OBJECT_SIZE, true );

	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT && selected )
	{
		const ApeLight *light = ( ApeLight * ) node;
		ape_draw_debug_sphere( node->position, PlColourF32ToU8( &light->colour ), light->radius );
	}
}

static void render_selected_objects( ApeEditorInstance *self )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_LINES );
	PlgImmSetPrimitiveScale( 2.0f );

	ApeWorldNode *worldNode;
	COM_ITERATE_LINKED_LIST( worldNode, self->selectedObjects, i )
	{
		switch ( worldNode->type )
		{
			default:
			{
				render_selected_wireframe( worldNode, &PL_COLOUR_BLUE, true );
				break;
			}
			case APE_WORLD_NODE_TYPE_BRUSH:
			{
				render_wireframe_brush( mesh, ( ApeBrush * ) worldNode, &PL_COLOUR_BLUE );
				break;
			}
		}
	}

	//todo: unify this somewhere and only fetch if cursor has moved
	self->hoverSelection = ape_editor_get_object_under_cursor( self );
	if ( self->hoverSelection != nullptr )
	{
		// sigh... this is going to depend on the type of node we're hovering
		worldNode = self->hoverSelection;
		if ( ape_world_node_has_magic( worldNode ) )
		{
			switch ( worldNode->type )
			{
				default:
				{
					render_selected_wireframe( worldNode, &PL_COLOUR_YELLOW, false );
					break;
				}
				case APE_WORLD_NODE_TYPE_BRUSH:
				{
					render_wireframe_brush( mesh, ( ApeBrush * ) worldNode, &PL_COLOUR_YELLOW );
					break;
				}
			}
		}
	}

	ape_material_draw( material, mesh, nullptr );
}

static void render_vertices( ApeEditorInstance *self )
{
	//TODO: only do this if the cursor has moved...
	self->hoverSelection = ape_editor_get_object_under_cursor( self );

	unsigned int   numVisibleNodes;
	ApeWorldNode **visibleNodes = ape_camera_get_visible_nodes_( self->camera, &numVisibleNodes );

	for ( unsigned int i = 0; i < numVisibleNodes; ++i )
	{
		const ApeWorldNode *node = visibleNodes[ i ];
		if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		ApeBrush *brush = ( ApeBrush * ) node;
		for ( unsigned int j = 0; j < brush->numVertices; ++j )
		{
			PLColour colour;
			if ( self->hoverSelection != nullptr && self->hoverSelection == &brush->vertices[ j ] )
			{
				colour = PL_COLOUR_YELLOW;
			}
			else
			{
				colour = PL_COLOUR_WHITE;
			}

			draw_selection_cube( &brush->vertices[ j ], &colour, SELECTION_VERTEX_SIZE, true );
		}
	}

	PLVector3 *vertex;
	COM_ITERATE_LINKED_LIST( vertex, self->selectedObjects, i )
	{
		draw_selection_cube( vertex, &PL_COLOUR_BLUE, SELECTION_VERTEX_SIZE, true );
	}
}

void ape_editor_selection_render_post_( ApeEditorInstance *self )
{
	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	switch ( self->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			ApeRoom *room = ape_camera_get_room( self->camera );
			if ( room != nullptr )
			{
				ape_room_draw_selected_( room, self );
			}

			render_selected_faces( self );
			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_VERTEX:
			render_vertices( self );
			render_transform_widget( self );
			break;
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
			render_selected_objects( self );
			render_transform_widget( self );
			break;
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
}

PLColour *ape_editor_get_pixel_under_cursor( PLColour *dst )
{
	ApeViewport    *selectionViewport = ape_editor_selection_get_viewport_();
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

void ape_editor_clear_selection( ApeEditorInstance *self )
{
	PlDestroyLinkedListNodes( self->selectedObjects );
}

void ape_editor_add_object_to_selection( ApeEditorInstance *self, void *object )
{
	// make sure it's not already selected...

	void *p;
	COM_ITERATE_LINKED_LIST( p, self->selectedObjects, i )
	{
		if ( object != p )
		{
			continue;
		}

		PlDestroyLinkedListNode( i );
		return;
	}

	PlInsertLinkedListNode( self->selectedObjects, object );
}

void *ape_editor_get_first_selected( ApeEditorInstance *self )
{
	PLLinkedListNode *node = PlGetFirstNode( self->selectedObjects );
	if ( node == nullptr )
	{
		return nullptr;
	}

	return PlGetLinkedListNodeUserData( node );
}

void ape_editor_delete_selection( ApeEditorInstance *self )
{
	ApeWorldNode *worldNode;
	COM_ITERATE_LINKED_LIST( worldNode, self->selectedObjects, i )
	{
		if ( !ape_world_node_has_magic( worldNode ) )
		{
			ape_warning_( "One of the selected items wasn't a valid world node, unable to delete!\n" );
			continue;
		}

		ape_world_node_destroy( worldNode );
	}

	ape_editor_clear_selection( self );
}

ApeViewport *ape_editor_selection_get_viewport_( void )
{
	return selectionViewport;
}

/////////////////////////////////////////////////////////////////////////////////////
// Transform Widget

static void render_transform_widget( ApeEditorInstance *instance )
{
	PLLinkedListNode *node = PlGetFirstNode( instance->selectedObjects );
	if ( node == nullptr )
	{
		return;
	}

	return;

	ApeWorldNode *selected = PlGetLinkedListNodeUserData( node );
	assert( selected != nullptr );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	if ( selected->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		ApeBrush *brush = ( ApeBrush * ) selected;
		assert( brush->numVertices > 0 );
		PlTranslateMatrix( brush->vertices[ 0 ] );
	}
	else
	{
		PLMatrix4 transform = ape_world_node_get_transform( selected );
		PLVector3 pos       = PlGetMatrix4Translation( &transform );
		PlTranslateMatrix( pos );
	}

	static constexpr float SCALE       = 32.0f;
	static constexpr float ARROW_SCALE = 4.0f;

	PlgImmBegin( PLG_MESH_LINES );

	PlgImmPushVertex( 0.0f, 0.0f, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );
	PlgImmPushVertex( SCALE, 0.0f, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );

	PlgImmPushVertex( SCALE, 0.0f, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );
	PlgImmPushVertex( SCALE - ARROW_SCALE, ARROW_SCALE, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );
	PlgImmPushVertex( SCALE, 0.0f, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );
	PlgImmPushVertex( SCALE - ARROW_SCALE, -ARROW_SCALE, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );

	PlgImmPushVertex( 0.0f, 0.0f, 0.0f );
	PlgImmColour( 0, 255, 0, 255 );
	PlgImmPushVertex( 0.0f, SCALE, 0.0f );
	PlgImmColour( 0, 255, 0, 255 );

	PlgImmPushVertex( 0.0f, SCALE, 0.0f );
	PlgImmColour( 0, 255, 0, 255 );
	PlgImmPushVertex( 0.0f, SCALE - ARROW_SCALE, ARROW_SCALE );
	PlgImmColour( 0, 255, 0, 255 );
	PlgImmPushVertex( 0.0f, SCALE, 0.0f );
	PlgImmColour( 0, 255, 0, 255 );
	PlgImmPushVertex( 0.0f, SCALE - ARROW_SCALE, -ARROW_SCALE );
	PlgImmColour( 0, 255, 0, 255 );

	PlgImmPushVertex( 0.0f, 0.0f, 0.0f );
	PlgImmColour( 0, 0, 255, 255 );
	PlgImmPushVertex( 0.0f, 0.0f, SCALE );
	PlgImmColour( 0, 0, 255, 255 );

	PlgImmPushVertex( 0.0f, 0.0f, SCALE );
	PlgImmColour( 0, 0, 255, 255 );
	PlgImmPushVertex( 0.0f, ARROW_SCALE, SCALE - ARROW_SCALE );
	PlgImmColour( 0, 0, 255, 255 );
	PlgImmPushVertex( 0.0f, 0.0f, SCALE );
	PlgImmColour( 0, 0, 255, 255 );
	PlgImmPushVertex( 0.0f, -ARROW_SCALE, SCALE - ARROW_SCALE );
	PlgImmColour( 0, 0, 255, 255 );

	PlgImmSetPrimitiveScale( 2.0f );
	PlgImmDraw();

	PlPopMatrix();
}
