// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: For handling selection buffer.

#include "plcore/pl_hashtable.h"
#include "plcore/pl_linkedlist.h"

#include "renderer/renderer.h"
#include "camera/camera.h"

#include "editor.h"
#include "renderer/renderer.h"
#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Selection Buffer
/////////////////////////////////////////////////////////////////////////////////////

static void setup_transform_widget();
static void cleanup_transform_widget();
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

	setup_transform_widget();
}

void ape_editor_selection_shutdown_()
{
	ape_viewport_destroy( selectionViewport );

	cleanup_transform_widget();
}

static QmMathColour4ub encode_hash_to_colour( uint64_t hash )
{
	return QM_MATH_COLOUR4UB( ( hash >> 16 ) & 0xFF, ( hash >> 8 ) & 0xFF, hash & 0xFF, 255 );
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
				PlInsertHashTableNode( self->selectionTable, &brush->faces[ i ].selectColour, sizeof( QmMathColour4ub ), &brush->faces[ i ] );
			}
		}
		else if ( mode == APE_EDITOR_GEOMETRY_MODE_VERTEX )
		{
			if ( brush->numVertices != brush->numVertexSelectColours )
			{
				brush->vertexSelectColours    = qm_os_memory_realloc( brush->vertexSelectColours, sizeof( QmMathColour4ub ) * brush->numVertices );
				brush->numVertexSelectColours = brush->numVertices;
			}
			for ( unsigned int i = 0; i < brush->numVertices; ++i )
			{
				brush->vertexSelectColours[ i ] = encode_hash_to_colour( ( uintptr_t ) &brush->vertices[ i ] );
				PlInsertHashTableNode( self->selectionTable, &brush->vertexSelectColours[ i ], sizeof( QmMathColour4ub ), &brush->vertices[ i ] );

				// this is dumb as hell, but throw it into our sub selection list too so we can determine the faces we need to update
				intptr_t ptr = ( intptr_t ) &brush->vertices[ i ];
				PlInsertHashTableNode( self->subSelectionTable, &ptr, sizeof( intptr_t ), brush );
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
	PlInsertHashTableNode( self->selectionTable, &node->selectColour, sizeof( QmMathColour4ub ), node );

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
	PlClearHashTable( self->subSelectionTable );
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
			const ApeBrushFaceVertex *vertex = &brush->faces[ i ].vertices[ brush->faces[ i ].edgeLoopOrder[ j ] ];
			PlgImmPushVertex( brush->vertices[ vertex->posIndex ].x,
			                  brush->vertices[ vertex->posIndex ].y,
			                  brush->vertices[ vertex->posIndex ].z );
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
			const ApeBrushFaceVertex *vertex = &brush->faces[ i ].vertices[ brush->faces[ i ].edgeLoopOrder[ j ] ];
			PlgImmPushVertex( brush->vertices[ vertex->posIndex ].x,
			                  brush->vertices[ vertex->posIndex ].y,
			                  brush->vertices[ vertex->posIndex ].z );
			PlgColour4bv( mesh, &brush->base.selectColour );
		}

		ape_material_draw( material, mesh, nullptr );
	}
}

static void draw_selection_cube( const QmMathVector3f *position, const QmMathColour4ub *colour, float scale, bool wireframe )
{
	if ( wireframe )
	{
		ape_draw_debug_aabb( &PL_COLLISION_AABB( *position, qm_math_vector3f( -scale, -scale, -scale ), qm_math_vector3f( scale, scale, scale ) ), *colour );
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
	for ( unsigned int i = 0; i < QM_OS_ARRAY_ELEMENTS( CUBE_INDICES ); ++i )
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

	if ( self->geometryMode != APE_EDITOR_GEOMETRY_MODE_PLOT )
	{
		for ( unsigned int i = 0; i < camera->pvs.rooms[ 0 ].numNodes; ++i )
		{
			const ApeWorldNode *node = camera->pvs.rooms[ 0 ].nodes[ i ];

			if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
			{
				switch ( node->type )
				{
					default:
						break;
					case APE_WORLD_NODE_TYPE_BRUSH:
						render_brush_selection( ( ApeBrush * ) node );
						break;
					case APE_WORLD_NODE_TYPE_MODEL:
					case APE_WORLD_NODE_TYPE_ENTITY:
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

	if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
	{
		// lights are throw into a different list, so we need to fetch them here
		for ( unsigned int i = 0; i < camera->pvs.rooms[ 0 ].numLights; ++i )
		{
			const ApeLight *light = camera->pvs.rooms[ 0 ].lights[ i ];
			assert( light != nullptr );

			QmMathVector3f pos = ape_light_get_position( light );
			draw_selection_cube( &pos, &light->base.selectColour, 8.0f, false );
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
		ApeBrush *brush = face->parent;
		assert( brush != nullptr );

		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			const ApeBrushFaceVertex *v0 = &face->vertices[ face->edgeLoopOrder[ j ] ];
			PlgImmPushVertex( brush->vertices[ v0->posIndex ].x,
			                  brush->vertices[ v0->posIndex ].y,
			                  brush->vertices[ v0->posIndex ].z );
			PlgImmColour( 0, 0, 255, 255 );

			const ApeBrushFaceVertex *v1 = j + 1 < face->numVertices ? &face->vertices[ face->edgeLoopOrder[ j + 1 ] ] : &face->vertices[ face->edgeLoopOrder[ 0 ] ];
			PlgImmPushVertex( brush->vertices[ v1->posIndex ].x,
			                  brush->vertices[ v1->posIndex ].y,
			                  brush->vertices[ v1->posIndex ].z );
			PlgImmColour( 0, 0, 255, 255 );
		}
	}

	if ( self->hoverSelection != nullptr )
	{
		face = self->hoverSelection;

		ApeBrush *brush = face->parent;
		assert( brush != nullptr );

		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			const ApeBrushFaceVertex *v0 = &face->vertices[ face->edgeLoopOrder[ i ] ];
			PlgImmPushVertex( brush->vertices[ v0->posIndex ].x,
			                  brush->vertices[ v0->posIndex ].y,
			                  brush->vertices[ v0->posIndex ].z );
			PlgImmColour( 255, 255, 0, 255 );

			const ApeBrushFaceVertex *v1 = i + 1 < face->numVertices ? &face->vertices[ face->edgeLoopOrder[ i + 1 ] ] : &face->vertices[ face->edgeLoopOrder[ 0 ] ];
			PlgImmPushVertex( brush->vertices[ v1->posIndex ].x,
			                  brush->vertices[ v1->posIndex ].y,
			                  brush->vertices[ v1->posIndex ].z );
			PlgImmColour( 255, 255, 0, 255 );
		}
	}

	ape_material_draw( material, mesh, nullptr );
}

static void render_wireframe_brush( PLGMesh *lineMesh, const ApeBrush *brush, const QmMathColour4ub *colour )
{
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		const ApeBrushFace *face = &brush->faces[ i ];
		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			const ApeBrushFaceVertex *v0 = &face->vertices[ face->edgeLoopOrder[ j ] ];
			PlgImmPushVertex( brush->vertices[ v0->posIndex ].x,
			                  brush->vertices[ v0->posIndex ].y,
			                  brush->vertices[ v0->posIndex ].z );
			PlgColour4bv( lineMesh, colour );

			const ApeBrushFaceVertex *v1 = j + 1 < face->numVertices ? &face->vertices[ face->edgeLoopOrder[ j + 1 ] ] : &face->vertices[ face->edgeLoopOrder[ 0 ] ];
			PlgImmPushVertex( brush->vertices[ v1->posIndex ].x,
			                  brush->vertices[ v1->posIndex ].y,
			                  brush->vertices[ v1->posIndex ].z );
			PlgColour4bv( lineMesh, colour );
		}
	}
}

static void render_selected_wireframe( ApeWorldNode *node, const QmMathColour4ub *colour, bool selected )
{
	draw_selection_cube( &node->position, colour, SELECTION_OBJECT_SIZE, true );
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

		if ( worldNode->classType->onDrawEditor != nullptr )
		{
			worldNode->classType->onDrawEditor( worldNode, true );
		}
	}

	//todo: unify this somewhere and only fetch if cursor has moved
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
	ApeCamera *camera = self->camera;
	for ( unsigned int i = 0; i < camera->pvs.rooms[ 0 ].numNodes; ++i )
	{
		const ApeWorldNode *node = camera->pvs.rooms[ 0 ].nodes[ i ];
		if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		ApeBrush *brush = ( ApeBrush * ) node;
		for ( unsigned int j = 0; j < brush->numVertices; ++j )
		{
			QmMathColour4ub colour;
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

	QmMathVector3f *vertex;
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
			//TODO: this doesn't work for vertices, yet!
			//render_transform_widget( self );
			break;
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
			render_selected_objects( self );
			render_transform_widget( self );
			break;
	}

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );
}

QmMathColour4ub *ape_editor_selection_get_pixel_under_cursor_( QmMathColour4ub *dst )
{
	ApeViewport    *selectionViewport = ape_editor_selection_get_viewport_();
	PLGFrameBuffer *frameBuffer       = ape_render_target_get_frame_buffer( selectionViewport->renderTarget );
	if ( frameBuffer == nullptr )
	{
		return nullptr;
	}

	size_t           size = frameBuffer->width * frameBuffer->height * 4;
	QmMathColour4ub *buf  = QM_OS_MEMORY_NEW_( QmMathColour4ub, size );
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
			qm_os_memory_free( buf );
			return dst;
		}
	}
	else
	{
		ape_warning_( "Failed to read framebuffer: %s\n", PlGetError() );
	}

	qm_os_memory_free( buf );
	return nullptr;
}

void *ape_editor_get_object_under_cursor( ApeEditorInstance *self )
{
	return self->hoverSelection;
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

	self->hoverSelection = nullptr;
}

void ape_editor_move_selection_to_room( ApeEditorInstance *self, ApeRoom *room )
{
	ApeWorldNode *worldNode;
	COM_ITERATE_LINKED_LIST( worldNode, self->selectedObjects, i )
	{
		if ( !ape_world_node_has_magic( worldNode ) )
		{
			ape_warning_( "One of the selected items wasn't a valid world node, unable to move!\n" );
			continue;
		}

		ape_world_node_attach( worldNode, APE_WORLD_NODE( room ) );
	}

	ape_editor_clear_selection( self );
}

ApeViewport *ape_editor_selection_get_viewport_( void )
{
	return selectionViewport;
}

/////////////////////////////////////////////////////////////////////////////////////
// Transform Widget

static PLGMesh *transformWidgetWireframeMesh;
static PLGMesh *transformWidgetSelectionMesh;

static void setup_transform_widget()
{
	static constexpr float TRANSFORM_SCALE       = 32.0f;
	static constexpr float TRANSFORM_ARROW_SCALE = 4.0f;

	transformWidgetWireframeMesh = PlgCreateMesh( PLG_MESH_LINES, PLG_DRAW_STATIC, 0, 18 );

	PlgSetMeshPrimitiveScale( transformWidgetWireframeMesh, 2.0f );

	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, 0.0f, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_RED );
	PlgPushVertex3f( transformWidgetWireframeMesh, TRANSFORM_SCALE, 0.0f, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_RED );

	PlgPushVertex3f( transformWidgetWireframeMesh, TRANSFORM_SCALE, 0.0f, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_RED );
	PlgPushVertex3f( transformWidgetWireframeMesh, TRANSFORM_SCALE - TRANSFORM_ARROW_SCALE, TRANSFORM_ARROW_SCALE, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_RED );
	PlgPushVertex3f( transformWidgetWireframeMesh, TRANSFORM_SCALE, 0.0f, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_RED );
	PlgPushVertex3f( transformWidgetWireframeMesh, TRANSFORM_SCALE - TRANSFORM_ARROW_SCALE, -TRANSFORM_ARROW_SCALE, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_RED );

	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, 0.0f, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_GREEN );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, TRANSFORM_SCALE, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_GREEN );

	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, TRANSFORM_SCALE, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_GREEN );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, TRANSFORM_SCALE - TRANSFORM_ARROW_SCALE, TRANSFORM_ARROW_SCALE );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_GREEN );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, TRANSFORM_SCALE, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_GREEN );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, TRANSFORM_SCALE - TRANSFORM_ARROW_SCALE, -TRANSFORM_ARROW_SCALE );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_GREEN );

	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, 0.0f, 0.0f );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_BLUE );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, 0.0f, TRANSFORM_SCALE );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_BLUE );

	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, 0.0f, TRANSFORM_SCALE );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_BLUE );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, TRANSFORM_ARROW_SCALE, TRANSFORM_SCALE - TRANSFORM_ARROW_SCALE );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_BLUE );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, 0.0f, TRANSFORM_SCALE );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_BLUE );
	PlgPushVertex3f( transformWidgetWireframeMesh, 0.0f, -TRANSFORM_ARROW_SCALE, TRANSFORM_SCALE - TRANSFORM_ARROW_SCALE );
	PlgColour4bv( transformWidgetWireframeMesh, &PL_COLOUR_BLUE );

	PlgUploadMesh( transformWidgetWireframeMesh );
}

static void cleanup_transform_widget()
{
	PlgDestroyMesh( transformWidgetWireframeMesh );
	PlgDestroyMesh( transformWidgetSelectionMesh );
}

static void render_transform_widget( ApeEditorInstance *instance )
{
	PLLinkedListNode *node = PlGetFirstNode( instance->selectedObjects );
	if ( node == nullptr )
	{
		return;
	}

	ApeWorldNode *selected = PlGetLinkedListNodeUserData( node );
	if ( selected == nullptr )
	{
		return;
	}

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
		PLMatrix4      transform = ape_world_node_get_transform( selected );
		QmMathVector3f pos       = PlGetMatrix4Translation( &transform );
		PlTranslateMatrix( pos );
	}

	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	ape_material_draw( material, transformWidgetWireframeMesh, nullptr );

	PlPopMatrix();
}
