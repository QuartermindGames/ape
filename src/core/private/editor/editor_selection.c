// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
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

static void add_node_to_face_selection( ApeEditorInstance *self, ApeWorldNode *node )
{
	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		ApeBrush *brush = ( ApeBrush * ) node;
		for ( uint i = 0; i < brush->numFaces; ++i )
		{
			uint64_t hash                  = PlGenerateHashFNV1( &brush->faces[ i ], sizeof( ApeBrushFace ) );
			brush->faces[ i ].selectColour = encode_hash_to_colour( hash );
			PlInsertHashTableNode( self->selectionTable, &brush->faces[ i ].selectColour, sizeof( PLColour ), &brush->faces[ i ] );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		add_node_to_face_selection( self, child );
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
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			add_node_to_face_selection( self, &room->base );
			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
		{
			add_node_to_transform_selection( self, &room->base );
			break;
		}
	}
}

static void render_node_selection( ApeEditorInstance *self, ApeWorldNode *node, ApeEditorGeometryMode mode )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	switch ( node->type )
	{
		default:
			break;
		case APE_WORLD_NODE_TYPE_BRUSH:
		{
			ApeBrush *brush = ( ApeBrush * ) node;
			if ( mode == APE_EDITOR_GEOMETRY_MODE_FACE )
			{
				//todo: optimize this... :(
				for ( uint i = 0; i < brush->numFaces; ++i )
				{
					PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
					for ( uint j = 0; j < brush->faces[ i ].numVertices; ++j )
					{
						const ApeBrushFaceVertex *vertex = brush->faces[ i ].edgeLoop[ j ];
						PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
						PlgColour4bv( mesh, &brush->faces[ i ].selectColour );
					}

					ape_material_draw( material, mesh, nullptr );
				}
			}
			else if ( mode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
			{
				//todo: optimize this... :(
				for ( uint i = 0; i < brush->numFaces; ++i )
				{
					PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
					for ( uint j = 0; j < brush->faces[ i ].numVertices; ++j )
					{
						const ApeBrushFaceVertex *vertex = brush->faces[ i ].edgeLoop[ j ];
						PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
						PlgColour4bv( mesh, &node->selectColour );
					}

					ape_material_draw( material, mesh, nullptr );
				}
			}
			break;
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		render_node_selection( self, child, mode );
	}
}

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

//#define DEBUG_GRID_SELECTION
#if !defined( DEBUG_GRID_SELECTION )
	ApeViewport *selectionViewport = ape_editor_selection_get_viewport_();

	uint sw = viewport->width / 2;
	uint sh = viewport->height / 2;
	ape_viewport_set_size( selectionViewport, sw, sh );
	ape_viewport_make_active( selectionViewport );
	ape_render_target_bind( selectionViewport->renderTarget, PLG_FRAMEBUFFER_DRAW );

	PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
#endif

	switch ( self->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_PLOT:
			ape_grid_draw_selection_( &self->grid );
			break;
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		case APE_EDITOR_GEOMETRY_MODE_VERTEX:
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
			render_node_selection( self, &room->base, self->geometryMode );
			break;
	}

#if !defined( DEBUG_GRID_SELECTION )
	ape_render_target_bind( viewport->renderTarget, PLG_FRAMEBUFFER_DEFAULT );
	ape_viewport_make_active( viewport );
#endif
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
