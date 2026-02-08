// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Lightmapper
// Author:  Mark E. Sowden

#include <float.h>

#include "qmos/public/qm_os_time.h"
#include "qmos/public/qm_os_random.h"
#include "qmmath/public/qm_math_plane.h"

#include "aux/public/aux_texture_packer.h"

#include "ape_private.h"

#include "game/game_public.h"

#include "world/world.h"
#include "renderer/renderer.h"
#include "renderer/renderer_texture.h"
#include "node/node_room.h"
#include "renderer/material/material.h"

/**
 * Some thoughts...
 *
 *	Lightmap per light. This will result in multiple passes, but will allow us to do
 *	specular etc.? Switching lights on the fly, or recomputing lightmaps at runtime should be cheaper...?
 *	Our biggest overhead right now are stencil shadows, though we're not caching so, go figure
 *
 *	Need to deal with concave faces, eventually, if we want to support things like
 *	the terrain etc., or, we finally decide on getting rid of support for concave faces
 *	and force convex
 */

static constexpr unsigned int LIGHTMAP_LUXEL_SIZE = 4;

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

	face->lightmapArea.x = ( float ) rect.x / APE_LIGHTMAP_WIDTH;
	face->lightmapArea.y = ( float ) rect.y / APE_LIGHTMAP_HEIGHT;
	face->lightmapArea.z = ( float ) ( rect.x + rect.w ) / APE_LIGHTMAP_WIDTH;
	face->lightmapArea.w = ( float ) ( rect.y + rect.h ) / APE_LIGHTMAP_HEIGHT;

	//TODO: this should be relative to luxel-size etc., blergh...
	static constexpr float PADDING = 0.002f;

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		QmMathVector2f p = get_projection( &brush->vertices[ face->vertices[ i ].posIndex ], projection );

		QmMathVector2f uv;
		uv = qm_math_vector2f_sub( p, min );
		uv = qm_math_vector2f_div( uv, qm_math_vector2f_sub( max, min ) );

		face->vertices[ i ].lightmapCoords.x = face->lightmapArea.x + PADDING + uv.x * ( face->lightmapArea.z - PADDING * 2.0f - face->lightmapArea.x );
		face->vertices[ i ].lightmapCoords.y = face->lightmapArea.y + PADDING + uv.y * ( face->lightmapArea.w - PADDING * 2.0f - face->lightmapArea.y );
	}

	return true;
}

static void generate_lightmap_( ApeRoom *room, const ApeBrushFace *face, ApeLight *light )
{
	if ( light->radius <= 0.0f )
	{
		return;
	}

	if ( !ape_light_test_face( light, face ) )
	{
		return;
	}

	unsigned int x = face->lightmapArea.x * APE_LIGHTMAP_WIDTH;
	unsigned int y = face->lightmapArea.y * APE_LIGHTMAP_HEIGHT;
	unsigned int w = ( face->lightmapArea.z - face->lightmapArea.x ) * APE_LIGHTMAP_WIDTH;
	unsigned int h = ( face->lightmapArea.w - face->lightmapArea.y ) * APE_LIGHTMAP_HEIGHT;

	const QmMathVector3f lightPos = ape_light_get_position( light );

	QmMathPlaneProjection projection = qm_math_plane_compute_projection( &( QmMathPlane ) {
	        .normal = face->normal,
	} );

	QmMathVector3f axis1, axis2;
	if ( projection == QM_MATH_PLANE_PROJECTION_XY )
	{
		axis1 = QM_MATH_PROJECT_XY[ 0 ];
		axis2 = QM_MATH_PROJECT_XY[ 1 ];
	}
	else if ( projection == QM_MATH_PLANE_PROJECTION_XZ )
	{
		axis1 = QM_MATH_PROJECT_XZ[ 0 ];
		axis2 = QM_MATH_PROJECT_XZ[ 1 ];
	}
	else
	{
		axis1 = QM_MATH_PROJECT_YZ[ 0 ];
		axis2 = QM_MATH_PROJECT_YZ[ 1 ];
	}

	QmMathVector3f faceOrigin = face->bounds.absOrigin;
	faceOrigin                = qm_math_vector3f_sub( faceOrigin, qm_math_vector3f_scale_float( axis1, w / 2.0f * LIGHTMAP_LUXEL_SIZE ) );
	faceOrigin                = qm_math_vector3f_sub( faceOrigin, qm_math_vector3f_scale_float( axis2, h / 2.0f * LIGHTMAP_LUXEL_SIZE ) );

	float planeDistance = -qm_math_vector3f_dot_product( face->normal, face->bounds.absOrigin );

	for ( unsigned int row = 0; row < h; ++row )
	{
		for ( unsigned int col = 0; col < w; ++col )
		{
			// need to translate this now into a world coord relative to the origin of the face, and w / h ...
			// sooo uh, need to convert the lightmap area relative to the luxel size I guess?

			// this should return the x and y in world units
			float fx = col * LIGHTMAP_LUXEL_SIZE + LIGHTMAP_LUXEL_SIZE / 2.0f;
			float fy = row * LIGHTMAP_LUXEL_SIZE + LIGHTMAP_LUXEL_SIZE / 2.0f;

			QmMathVector3f luxelPos = faceOrigin;
			luxelPos                = qm_math_vector3f_add( luxelPos, qm_math_vector3f_scale_float( axis1, fx ) );
			luxelPos                = qm_math_vector3f_add( luxelPos, qm_math_vector3f_scale_float( axis2, fy ) );

			// this attempts to reproject it, so we can deal with angled surfaces
			// though this isn't perfect, it's 2D reprojected to 3D so the luxel span isn't correct anymore (aaarrrghhh)
			float sd = qm_math_vector3f_dot_product( face->normal, luxelPos ) + planeDistance;
			luxelPos = qm_math_vector3f_sub( luxelPos, qm_math_vector3f_scale_float( face->normal, sd ) );

			QmMathVector3f lightDir = qm_math_vector3f_sub( luxelPos, lightPos );
			lightDir                = qm_math_vector3f_normalize( lightDir );

			//ape_draw_debug_axis( luxelPos, QM_MATH_VECTOR3F_ZERO, 2.0f );

#if 1
			ApeMaterial *material = face->material;
			if ( ape_material_can_receive_shadows( material ) && light->flags & APE_LIGHT_FLAG_SHADOWS )
			{
				PLCollisionRay ray = {};
				ray.origin         = lightPos;
				ray.direction      = lightDir;

				ApeCollisionIntersection result = {};
				if ( !ape_room_ray_intersect( room, &ray, &result ) || result.face == nullptr )
				{
					continue;
				}

				if ( result.face != face )
				{
					material = result.face->material;
					if ( ape_material_can_cast_shadows( material ) )
					{
						continue;
					}

					if ( !ape_material_is_blended( material ) )
					{
						continue;
					}

					//TODO: handle blended surfaces, refraction, yadda yadda
				}
			}

			//ape_draw_debug_line( lightPos, result.intersection, PL_COLOUR_GREEN );
#endif

			// just pulled much of the below from our existing shaders...

			float d = qm_math_vector3f_distance( lightPos, luxelPos );
			float r = QM_MATH_CLAMP( 0.0f, 1.0f - d * d / ( light->radius * light->radius ), 1.0f );
			float l = QM_OS_MAX( qm_math_vector3f_dot_product( face->normal, lightDir ), 1.0f );

			QmMathVector3f c;
			c = qm_math_vector3f_scale_float( qm_math_vector3f( light->colour.r, light->colour.g, light->colour.b ), l * light->colour.a );
			c = qm_math_vector3f_scale_float( c, r );

			ApeLightmapPixel *pixel = &room->lightmap[ ( y + row ) * APE_LIGHTMAP_WIDTH + ( x + col ) ];

#if defined( APE_RENDERER_LIGHTMAP_USE_FLOATS )
			pixel->colour.r += c.x;
			pixel->colour.g += c.y;
			pixel->colour.b += c.z;
#else
			unsigned int cr = pixel->colour.r;
			unsigned int cg = pixel->colour.g;
			unsigned int cb = pixel->colour.b;

			cr = QM_MATH_CLAMP( 0, cr + ( unsigned int ) QM_MATH_FTOB( c.x ), 255 );
			cg = QM_MATH_CLAMP( 0, cg + ( unsigned int ) QM_MATH_FTOB( c.y ), 255 );
			cb = QM_MATH_CLAMP( 0, cb + ( unsigned int ) QM_MATH_FTOB( c.z ), 255 );

			pixel->colour.r = cr;
			pixel->colour.g = cg;
			pixel->colour.b = cb;
#endif
		}
	}
}

static void gather_nodes( ApeWorldNode *node, QmOsLinkedList *lights, QmOsLinkedList *brushes )
{
	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		ApeLight *light = ( ApeLight * ) node;
		if ( !( light->flags & APE_LIGHT_FLAG_DYNAMIC ) )
		{
			qm_os_linked_list_push_back( lights, light );
		}
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

	AuxTexturePackerNode *lightmapPacker = aux_texture_packer_node_create_root( APE_LIGHTMAP_WIDTH, APE_LIGHTMAP_HEIGHT );
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
		room->lightmap = QM_OS_MEMORY_NEW_( ApeLightmapPixel, APE_LIGHTMAP_WIDTH * APE_LIGHTMAP_HEIGHT );
		if ( room->lightmap == nullptr )
		{
			ape_console_warning_( "Failed to allocate lightmap!" );
			goto cleanup;
		}
	}

	// setup ambience first
	for ( unsigned int i = 0; i < APE_LIGHTMAP_PIXELS; ++i )
	{
		room->lightmap[ i ].colour.r = room->ambientLight.r;
		room->lightmap[ i ].colour.g = room->ambientLight.g;
		room->lightmap[ i ].colour.b = room->ambientLight.b;
	}

	ApeBrush *brush;
	QM_OS_LINKED_LIST_ITERATE( brush, brushes, i )
	{
		for ( unsigned int k = 0; k < brush->numFaces; ++k )
		{
			//if ( brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_HIDDEN ||
			//     brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_MIRROR )
			//{
			//	continue;
			//}

			// if we're dealing with a portal, we want to navigate down
			// to figure out what else we need to deal with
			//if ( brush->faces[ k ].flags & APE_BRUSH_FACE_FLAG_PORTAL )
			//{
			//	ApeBrushFace *dstFace = ape_brush_face_get_portal_destination( &brush->faces[ k ] );
			//	if ( dstFace != &brush->faces[ k ] )
			//	{
			//		//TODO: navigate through portals, add to list, handle recursion, wheeee
			//		continue;
			//	}
			//
			//	//TODO: remove
			//	continue;
			//}

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
		ApeLight *light;
		QM_OS_LINKED_LIST_ITERATE( light, lights, i )
		{
			generate_lightmap_( room, face, light );

#if 0// a little experiment...
			static constexpr unsigned int SAMPLES = 8;

			unsigned int seed = SAMPLES;

			QmMathVector3f storePos   = light->base.position;
			float          storePower = light->colour.a;

			for ( unsigned int j = 0; j < SAMPLES; ++j )
			{
#	define JITTER_VARIATION ( qm_os_random_float( &seed, ( ( float ) j ) * ( SAMPLES * 2.0f ) / SAMPLES ) - \
		                       qm_os_random_float( &seed, ( ( float ) j ) * ( SAMPLES * 2.0f ) / SAMPLES ) )
				light->base.position.x += JITTER_VARIATION;
				light->base.position.y += JITTER_VARIATION;
				light->base.position.z += JITTER_VARIATION;
				light->colour.a = j * 1.0f / SAMPLES;

				generate_lightmap_( room, face, light );
			}

			light->base.position = storePos;
			light->colour.a      = storePower;
#endif
		}
	}

	// convert the lightmap into a texture we can use
	ape_room_upload_lightmap_( room, APE_LIGHTMAP_WIDTH, APE_LIGHTMAP_HEIGHT );

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

static bool display_brush_uv( ApeWorldNode *node, void *user )
{
	const float scale = *( float * ) user;

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

			QmMathVector2f SCALE = QM_MATH_VECTOR2F( ( float ) APE_LIGHTMAP_WIDTH * scale, ( float ) APE_LIGHTMAP_HEIGHT * scale );

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

	ApeMaterial *debugLightmapMaterial = ape_material_cache( "materials/debug/debug_lightmap.mat.n", APE_CACHE_GROUP_GLOBAL, false );
	if ( debugLightmapMaterial == nullptr )
	{
		return;
	}

	static float SCALE = 2.0f;

	float w = ( float ) APE_LIGHTMAP_WIDTH * SCALE;
	float h = ( float ) APE_LIGHTMAP_HEIGHT * SCALE;

	ape_rendererState_.lightmapTexture = room->lightmapTexture->internal;
	ape_draw_textured_quad( debugLightmapMaterial, 0.0f, 0.0f, w, h, &PL_COLOUR_WHITE, 0.0f );
	ape_rendererState_.lightmapTexture = nullptr;

	ape_world_node_visit_children( APE_WORLD_NODE( room ), APE_WORLD_NODE_TYPE_BRUSH, true, display_brush_uv, &SCALE );

	ape_material_release( debugLightmapMaterial );
}
