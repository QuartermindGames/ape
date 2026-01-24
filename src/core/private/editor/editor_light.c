// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Lightmapper
// Author:  Mark E. Sowden

#include <float.h>

#include "qmos/public/qm_os_time.h"
#include "qmmath/public/qm_math_plane.h"

#include "aux/public/aux_texture_packer.h"

#include "ape_private.h"

#include "game/game_public.h"

#include "world/world.h"
#include "renderer/renderer.h"

/**
 * Some thoughts...
 *
 *	Lightmap per light. This will result in multiple passes, but will allow us to do
 *	specular etc.? Switching lights on the fly, or recomputing lightmaps at runtime should be cheaper...?
 *	Our biggest overhead right now are stencil shadows, though we're not caching so, go figure
 *
 *	Consider moving this into the cook tool?
 *	Should the cook tool be turned into a library?
 */

static constexpr unsigned int LIGHTMAP_TEXTURE_WIDTH  = 512;
static constexpr unsigned int LIGHTMAP_TEXTURE_HEIGHT = 512;

static constexpr unsigned int LIGHTMAP_LUXEL_SIZE = 4;

static constexpr char LIGHTMAP_EXTENSION[] = ".lmp";

static AuxTexturePackerNode *lightmapCache;

static void setup_face_lightmap( ApeBrushFace *face, const ApeBrush *brush )
{
	// first calculate the bounds

	QmMathPlaneProjection projection = qm_math_plane_compute_projection( &( QmMathPlane ) {
	        .normal = face->normal,
	} );

	QmMathVector2f min = QM_MATH_VECTOR2F( FLT_MAX, FLT_MAX );
	QmMathVector2f max = QM_MATH_VECTOR2F( -FLT_MAX, -FLT_MAX );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		QmMathVector2f p = {};
		switch ( projection )
		{
			case QM_MATH_PLANE_PROJECTION_YZ:
				p.x = brush->vertices[ face->vertices[ i ].posIndex ].y;
				p.y = brush->vertices[ face->vertices[ i ].posIndex ].z;
				break;
			case QM_MATH_PLANE_PROJECTION_XZ:
				p.x = brush->vertices[ face->vertices[ i ].posIndex ].x;
				p.y = brush->vertices[ face->vertices[ i ].posIndex ].z;
				break;
			case QM_MATH_PLANE_PROJECTION_XY:
				p.x = brush->vertices[ face->vertices[ i ].posIndex ].x;
				p.y = brush->vertices[ face->vertices[ i ].posIndex ].y;
				break;
		}

		if ( p.x < min.x )
		{
			min.x = p.x;
		}
		if ( p.y < min.y )
		{
			min.y = p.y;
		}

		if ( p.x > max.x )
		{
			max.x = p.x;
		}
		if ( p.y > max.y )
		{
			max.y = p.y;
		}
	}

	// need to figure out where it's going to fit into our lightmap sheet

	unsigned int w = ( unsigned int ) ceilf( ( max.x - min.x ) / LIGHTMAP_LUXEL_SIZE );
	unsigned int h = ( unsigned int ) ceilf( ( max.y - min.y ) / LIGHTMAP_LUXEL_SIZE );

	AuxTexturePackerNode *node = aux_texture_packer_node_insert( lightmapCache, w, h );
	if ( node == nullptr )
	{
		//TODO: if this occurs, it might need to go into another sheet
		//		but we'll worry about that later
		ape_console_warning_( "Failed to insert node into lightmap (%ux%u)!\n", w, h );
		return;
	}

	// now update the uv so its correct relative to where it's going to be on the sheet

	ComMathRectI32 rect = aux_texture_packer_node_get_rect( node );

	QmMathVector2f sx;
	sx.x = ( float ) rect.x / LIGHTMAP_TEXTURE_WIDTH;
	sx.y = ( float ) rect.y / LIGHTMAP_TEXTURE_HEIGHT;

	QmMathVector2f sy;
	sy.x = ( float ) ( rect.x + rect.w ) / LIGHTMAP_TEXTURE_WIDTH;
	sy.y = ( float ) ( rect.y + rect.h ) / LIGHTMAP_TEXTURE_HEIGHT;

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		QmMathVector2f p = {};
		switch ( projection )
		{
			case QM_MATH_PLANE_PROJECTION_YZ:
				p.x = brush->vertices[ face->vertices[ i ].posIndex ].y;
				p.y = brush->vertices[ face->vertices[ i ].posIndex ].z;
				break;
			case QM_MATH_PLANE_PROJECTION_XZ:
				p.x = brush->vertices[ face->vertices[ i ].posIndex ].x;
				p.y = brush->vertices[ face->vertices[ i ].posIndex ].z;
				break;
			case QM_MATH_PLANE_PROJECTION_XY:
				p.x = brush->vertices[ face->vertices[ i ].posIndex ].x;
				p.y = brush->vertices[ face->vertices[ i ].posIndex ].y;
				break;
		}

		QmMathVector2f uv;
		uv = qm_math_vector2f_sub( p, min );
		uv = qm_math_vector2f_div( uv, qm_math_vector2f_sub( max, min ) );

		face->vertices[ i ].lightmapCoords.x = sx.x + uv.x * ( sy.x - sx.x );
		face->vertices[ i ].lightmapCoords.y = sx.y + uv.y * ( sy.y - sx.y );
	}
}

static void generate_lightmap_( ApeLight *light )
{
	if ( light->lightmap == nullptr )
	{
		light->lightmap = QM_OS_MEMORY_NEW_( ApeLightmapPixel, APE_LIGHTMAP_SIZE * APE_LIGHTMAP_SIZE );
	}
	else
	{
		PL_ZERO( light->lightmap, APE_LIGHTMAP_BUFFER_SIZE );
	}

	float step = 1.0f / ( float ) APE_LIGHTMAP_SIZE;
	for ( unsigned int i = 0; i < APE_LIGHTMAP_SIZE; ++i )
	{
		for ( unsigned int j = 0; j < APE_LIGHTMAP_SIZE; ++j )
		{
		}
	}
}

static void gather_nodes( ApeWorldNode *node, QmOsLinkedList *lights, QmOsLinkedList *brushes )
{
	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		//TODO: for now, for the sake of testing, we're ignoring this
		//if ( !( light->flags & APE_LIGHT_FLAG_DYNAMIC ) )
		ApeLight *light = ( ApeLight * ) node;
		qm_os_linked_list_push_back( lights, light );
	}
	else if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		qm_os_linked_list_push_back( brushes, node );
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		gather_nodes( child, lights, brushes );
	}
}

void ape_editor_light_generate_( ApeRoom *room )
{
	ape_console_print_( "Generating lightmap...\n" );

	double startTime = qm_os_time_get_seconds();

	lightmapCache = aux_texture_packer_node_create_root( LIGHTMAP_TEXTURE_WIDTH, LIGHTMAP_TEXTURE_HEIGHT );

	// first, gather all the lights for the given room we need to operate on

	QmOsLinkedList *lights  = qm_os_linked_list_create();
	QmOsLinkedList *brushes = qm_os_linked_list_create();
	if ( lights == nullptr || brushes == nullptr )
	{
		ape_console_warning_( "Failed to create lists for lightmap generation!\n" );
		goto cleanup;
	}

	gather_nodes( APE_WORLD_NODE( room ), lights, brushes );

	ape_console_print_( "Processing %u lights, %u brushes...\n",
	                    qm_os_linked_list_get_size( lights ),
	                    qm_os_linked_list_get_size( brushes ) );

	ApeBrush *brush;
	QM_OS_LINKED_LIST_ITERATE( brush, brushes, i )
	{
		for ( unsigned int k = 0; k < brush->numFaces; ++k )
		{
			// we probably don't want lightmaps for mirror or portal faces for now?
			// mirrors are still doable if you've got a material assigned with mirror prop
			if ( brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_HIDDEN ||
			     brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_MIRROR ||
			     brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_PORTAL )
			{
				continue;
			}

			setup_face_lightmap( &brush->faces[ k ], brush );

			// mark it dirty so we reupload later with the new uv
			ape_brush_mark_parent_dirty( brush );
		}
	}

	// now, generate the lightmap for each light
	ApeLight *light;
	QM_OS_LINKED_LIST_ITERATE( light, lights, i )
	{
		generate_lightmap_( light );
	}

	double endTime = qm_os_time_get_seconds();
	ape_console_print_( "Lightmap generation took %.3f seconds.\n", endTime - startTime );

cleanup:
	qm_os_memory_free( lights );
	qm_os_memory_free( brushes );
}

void ape_light_command_( unsigned int, char ** )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->camera == nullptr )
	{
		ape_console_warning_( "Unable to generate lightmap, invalid editor instance!\n" );
		return;
	}

	//TODO: this should operate over all rooms open, not just wherever the camera is!
	//		why do we not have a get_world method for ed?
	ApeRoom *room = ape_camera_get_room( instance->camera );
	if ( room == nullptr )
	{
		ape_console_warning_( "Unable to generate lightmap, no valid camera!\n" );
		return;
	}

	ape_editor_light_generate_( room );
}
