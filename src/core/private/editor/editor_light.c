// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Lightmapper
// Author:  Mark E. Sowden

#include <float.h>

#include "qmos/public/qm_os_time.h"
#include "qmmath/public/qm_math_plane.h"

#include "aux/public/aux_texture_packer.h"

#include "ape_private.h"

#include "game/game_public.h"
#include "qmos/public/qm_os_random.h"

#include "world/world.h"
#include "renderer/renderer.h"
#include "renderer/renderer_texture.h"

/**
 * Some thoughts...
 *
 *	Lightmap per light. This will result in multiple passes, but will allow us to do
 *	specular etc.? Switching lights on the fly, or recomputing lightmaps at runtime should be cheaper...?
 *	Our biggest overhead right now are stencil shadows, though we're not caching so, go figure
 *
 *	Consider moving this into the cook tool?
 *	Should the cook tool be turned into a library?
 *
 *	Store min/max rect for face as area?
 *	float w, h - relative to origin
 */

static constexpr unsigned int LIGHTMAP_WIDTH       = 512;
static constexpr unsigned int LIGHTMAP_HEIGHT      = 512;
static constexpr unsigned int LIGHTMAP_BUFFER_SIZE = LIGHTMAP_HEIGHT * LIGHTMAP_WIDTH * sizeof( ApeLightmapPixel );

static constexpr unsigned int LIGHTMAP_LUXEL_SIZE = 8;

static constexpr char LIGHTMAP_EXTENSION[] = ".lmp";

static QmMathVector2f get_projection( const QmMathVector3f *point, QmMathPlaneProjection projection )
{
	if ( projection == QM_MATH_PLANE_PROJECTION_XY )
	{
		return qm_math_vector2f( point->x, point->y );
	}

	if ( projection == QM_MATH_PLANE_PROJECTION_XZ )
	{
		return qm_math_vector2f( point->x, point->z );
	}

	return qm_math_vector2f( point->y, point->z );
}

static bool setup_face_lightmap( ApeBrushFace *face, const ApeBrush *brush, AuxTexturePackerNode *lightmapPacker )
{
	// first calculate the bounds

	QmMathPlaneProjection projection = qm_math_plane_compute_projection( &( QmMathPlane ) {
	        .normal = face->normal,
	} );

	QmMathVector2f min = QM_MATH_VECTOR2F( FLT_MAX, FLT_MAX );
	QmMathVector2f max = QM_MATH_VECTOR2F( -FLT_MAX, -FLT_MAX );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		QmMathVector2f p = get_projection( &brush->vertices[ face->vertices[ i ].posIndex ], projection );

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

	AuxTexturePackerNode *node = aux_texture_packer_node_insert( lightmapPacker, w, h );
	if ( node == nullptr )
	{
		//TODO: if this occurs, it might need to go into another sheet
		//		but we'll worry about that later
		ape_console_warning_( "Failed to insert node into lightmap (%ux%u)!\n", w, h );
		return false;
	}

	// now update the uv so its correct relative to where it's going to be on the sheet

	ComMathRectI32 rect = aux_texture_packer_node_get_rect( node );

	face->lightmapArea.x = ( float ) rect.x / LIGHTMAP_WIDTH;
	face->lightmapArea.y = ( float ) rect.y / LIGHTMAP_HEIGHT;
	face->lightmapArea.z = ( float ) ( rect.x + rect.w ) / LIGHTMAP_WIDTH;
	face->lightmapArea.w = ( float ) ( rect.y + rect.h ) / LIGHTMAP_HEIGHT;

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		QmMathVector2f p = get_projection( &brush->vertices[ face->vertices[ i ].posIndex ], projection );

		QmMathVector2f uv;
		uv = qm_math_vector2f_sub( p, min );
		uv = qm_math_vector2f_div( uv, qm_math_vector2f_sub( max, min ) );

		face->vertices[ i ].lightmapCoords.x = face->lightmapArea.x + uv.x * ( face->lightmapArea.z - face->lightmapArea.x );
		face->vertices[ i ].lightmapCoords.y = face->lightmapArea.y + uv.y * ( face->lightmapArea.w - face->lightmapArea.y );
	}

	return true;
}

static void generate_lightmap_( ApeRoom *room, ApeBrushFace *face, ApeLight *light )
{
	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	unsigned int seed = ( uintptr_t ) face;

	QmMathColour4ub colour;
	colour.r = 128 + qm_os_random_int( &seed ) % 128 - 1;
	colour.g = 128 + qm_os_random_int( &seed ) % 128 - 1;
	colour.b = 128 + qm_os_random_int( &seed ) % 128 - 1;
	colour.a = 255;

	unsigned int x = face->lightmapArea.x * LIGHTMAP_WIDTH;
	unsigned int y = face->lightmapArea.y * LIGHTMAP_HEIGHT;
	unsigned int w = ( face->lightmapArea.z - face->lightmapArea.x ) * LIGHTMAP_WIDTH;
	unsigned int h = ( face->lightmapArea.w - face->lightmapArea.y ) * LIGHTMAP_HEIGHT;

	for ( unsigned int row = 0; row < h; ++row )
	{
		for ( unsigned int col = 0; col < w; ++col )
		{
			ApeLightmapPixel *pixel = &room->lightmap[ ( y + row ) * LIGHTMAP_WIDTH + ( x + col ) ];
			pixel->colour.r         = colour.r;
			pixel->colour.g         = colour.g;
			pixel->colour.b         = colour.b;
		}
	}
}

static void gather_nodes( ApeWorldNode *node, QmOsLinkedList *lights, QmOsLinkedList *brushes )
{
	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		ApeLight *light = ( ApeLight * ) node;
		//TODO: for now, for the sake of testing, we're ignoring this
		//if ( !( light->flags & APE_LIGHT_FLAG_DYNAMIC ) )
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

	AuxTexturePackerNode *lightmapPacker = aux_texture_packer_node_create_root( LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT );
	if ( lightmapPacker == nullptr )
	{
		ape_console_warning_( "Failed to setup lightmap packer!\n" );
		return;
	}

	// first, gather all the objects for the given room we need to operate on

	QmOsLinkedList *lights  = qm_os_linked_list_create();
	QmOsLinkedList *brushes = qm_os_linked_list_create();
	QmOsLinkedList *faces   = qm_os_linked_list_create();
	if ( lights == nullptr || brushes == nullptr || faces == nullptr )
	{
		ape_console_warning_( "Failed to create lists for lightmap generation!\n" );
		goto cleanup;
	}

	double startTime = qm_os_time_get_seconds();

	gather_nodes( APE_WORLD_NODE( room ), lights, brushes );

	ape_console_print_( "Processing %u lights, %u brushes...\n",
	                    qm_os_linked_list_get_size( lights ),
	                    qm_os_linked_list_get_size( brushes ) );

	if ( room->lightmap == nullptr )
	{
		room->lightmap = QM_OS_MEMORY_NEW_( ApeLightmapPixel, LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT );
		if ( room->lightmap == nullptr )
		{
			ape_console_warning_( "Failed to allocate lightmap!" );
			goto cleanup;
		}
	}
	else
	{
		memset( room->lightmap, 0, LIGHTMAP_BUFFER_SIZE );
	}

	ApeBrush *brush;
	QM_OS_LINKED_LIST_ITERATE( brush, brushes, i )
	{
		for ( unsigned int k = 0; k < brush->numFaces; ++k )
		{
			if ( brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_HIDDEN ||
			     brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_MIRROR )
			{
				continue;
			}

			// if we're dealing with a portal, we want to navigate down
			// to figure out what else we need to deal with
			if ( brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_PORTAL )
			{
				ApeBrushFace *dstFace = ape_brush_face_get_portal_destination( &brush->faces[ k ] );
				if ( dstFace != &brush->faces[ k ] )
				{
					//TODO: navigate through portals, add to list, handle recursion, wheeee
					continue;
				}

				//TODO: remove
				continue;
			}

			if ( !setup_face_lightmap( &brush->faces[ k ], brush, lightmapPacker ) )
			{
				continue;
			}

			// add it to a list so we can quickly iterate over it later
			qm_os_linked_list_push_back( faces, &brush->faces[ k ] );

			// mark it dirty so we reupload later with the new uv
			ape_brush_mark_parent_dirty( brush );
		}
	}

	ApeBrushFace *face;
	QM_OS_LINKED_LIST_ITERATE( face, faces, i )
	{
		// now, generate the lightmap for each light
		//ApeLight *light;
		//QM_OS_LINKED_LIST_ITERATE( light, lights, i )
		//{
		generate_lightmap_( room, face, nullptr );
		//}
	}

	// convert the lightmap into a texture we can use
	if ( room->lightmapTexture != nullptr )
	{
		ape_texture_release_( room->lightmapTexture );
		ape_memory_flush_unreferenced_resources();
	}

	room->lightmapTexture = ape_texture_generate_( "lightmap", room->lightmap, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, &QM_IMAGE_FORMAT_RGB8_DESC(), false );
	if ( room->lightmapTexture == nullptr )
	{
		ape_console_warning_( "Failed to create lightmap texture!\n" );
	}

	double endTime = qm_os_time_get_seconds();
	ape_console_print_( "Lightmap generation took %.3f seconds.\n", endTime - startTime );

cleanup:
	qm_os_memory_free( lights );
	qm_os_memory_free( brushes );
	qm_os_memory_free( faces );
	qm_os_memory_free( lightmapPacker );
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

#if defined( APE_COMPILE_TESTS )

static bool display_brush_uv( ApeWorldNode *node, void *user )
{
	ApeBrush *brush = ( ApeBrush * ) node;
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		ApeBrushFace *face = &brush->faces[ i ];

		unsigned int seed = ( uintptr_t ) face;

		QmMathColour4ub colour;
		colour.r = 128 + qm_os_random_int( &seed ) % 128 - 1;
		colour.g = 128 + qm_os_random_int( &seed ) % 128 - 1;
		colour.b = 128 + qm_os_random_int( &seed ) % 128 - 1;
		colour.a = 255;

		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			unsigned int   k     = ( j + 1 ) % face->numVertices;
			QmMathVector2f start = face->vertices[ face->edgeLoopOrder[ j ] ].lightmapCoords;
			QmMathVector2f end   = face->vertices[ face->edgeLoopOrder[ k ] ].lightmapCoords;

			static constexpr QmMathVector2f SCALE = QM_MATH_VECTOR2F( LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT );

			start = qm_math_vector2f_scale( start, SCALE );
			end   = qm_math_vector2f_scale( end, SCALE );

			PlgImmBegin( PLG_MESH_LINES );
			PlgImmPushVertex( start.x, start.y, 1.0f );
			PlgImmColour( colour.r, colour.g, colour.b, 255 );
			PlgImmPushVertex( end.x, end.y, 1.0f );
			PlgImmColour( colour.r, colour.g, colour.b, 255 );
			PlgImmDraw();
		}
	}

	return true;
}

/**
 * Quick dirty function to display the UV map on screen for troubleshooting.
 */
void ape_editor_light_display_lightmap_overlay_( const ApeEditorInstance *instance )
{
	ApeCamera *camera = instance->camera;
	if ( camera == nullptr )
	{
		return;
	}

	ApeRoom *room = ape_camera_get_room( camera );
	if ( room == nullptr || room->lightmapTexture == nullptr )
	{
		return;
	}

	static ApeMaterial *debugLightmapMaterial;
	if ( debugLightmapMaterial == nullptr )
	{
		debugLightmapMaterial = ape_material_cache( "materials/debug/debug_lightmap.mat.n", APE_CACHE_GROUP_GLOBAL, true );
	}

	float w = ( float ) LIGHTMAP_WIDTH;
	float h = ( float ) LIGHTMAP_HEIGHT;

	ape_rendererState_.lightmapTexture = room->lightmapTexture->internal;
	ape_draw_textured_quad( debugLightmapMaterial, 0.0f, 0.0f, w, h, &PL_COLOUR_WHITE, 0.0f );
	ape_rendererState_.lightmapTexture = nullptr;

	ape_world_node_visit_children( APE_WORLD_NODE( room ), APE_WORLD_NODE_TYPE_BRUSH, true, display_brush_uv, nullptr );
}

#endif//APE_COMPILE_TESTS
