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
#include "renderer/material/material.h"

#include "node/node_room.h"

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

/////////////////////////////////////////////////////////////////////////////////////
// Lightmap
/////////////////////////////////////////////////////////////////////////////////////

static void lightmap_clear( ApeLightmap *self, const QmMathColour3f *colour, unsigned int edgeLength )
{
	unsigned int size = edgeLength * edgeLength;
	for ( unsigned int i = 0; i < size; ++i )
	{
		self->pixels[ i ].colour.r = colour->r;
		self->pixels[ i ].colour.g = colour->g;
		self->pixels[ i ].colour.b = colour->b;
	}
}

ApeLightmap *ape_lightmap_create_( unsigned int edgeLength )
{
	ApeLightmap *lightmap = QM_OS_MEMORY_NEW( ApeLightmap );
	if ( lightmap == nullptr )
	{
		ape_console_warning_( "Failed to allocate lightmap!\n" );
		return nullptr;
	}

	lightmap->pixels = QM_OS_MEMORY_NEW_( ApeLightmapPixel, edgeLength * edgeLength );
	if ( lightmap->pixels == nullptr )
	{
		ape_console_warning_( "Failed to allocate lightmap pixel buffer!\n" );
		return nullptr;
	}

	return lightmap;
}

void ape_lightmap_destroy_( ApeLightmap *self )
{
	ape_texture_release_( self->texture );
	ape_memory_flush_unreferenced_resources();

	qm_os_memory_free( self->pixels );
	qm_os_memory_free( self->packer );
	qm_os_memory_free( self );
}

void ape_lightmap_upload_( ApeLightmap *self, unsigned int edgeLength )
{
	if ( self->texture != nullptr )
	{
		ape_texture_release_( self->texture );
		ape_memory_flush_unreferenced_resources();
	}

	self->texture = ape_texture_generate_( "lightmap", self->pixels, edgeLength, edgeLength, &QM_IMAGE_FORMAT_RGB16F_DESC(), PLG_TEXTURE_FILTER_LINEAR );
	if ( self->texture == nullptr )
	{
		ape_console_warning_( "Failed to create lightmap texture!\n" );
		return;
	}

	//TODO: remove this!!! ITS A BOTCH - this should be updated by the material draw method, probably
	self->texture->wrapMode = PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE;
	qm_gfx_texture_set_wrap_mode( self->texture->internal, self->texture->wrapMode );
}

void ape_lightmap_serialize_( const ApeLightmap *self, unsigned int edgeLength, AcmBranch *root )
{
	AcmBranch *pixelsBranch = acm_push_array_f16( root, "pixels", nullptr, 0 );

	unsigned int lightmapSize = edgeLength * edgeLength;
	for ( unsigned int i = 0; i < lightmapSize; ++i )
	{
		ApeLightmapPixel *pixel = &self->pixels[ i ];
		for ( unsigned int j = 0; j < 3; ++j )
		{
			acm_push_f16( pixelsBranch, nullptr, pixel->colour.v[ j ] );
		}
	}
}

ApeLightmap *ape_lightmap_deserialize_( unsigned int edgeLength, AcmBranch *root )
{
	ApeLightmap *lightmap = ape_lightmap_create_( edgeLength );
	if ( lightmap == nullptr )
	{
		return nullptr;
	}

	unsigned int lightmapSize = edgeLength * edgeLength;

	AcmBranch *pixelsBranch = acm_get_child_by_name( root, "pixels" );
	if ( pixelsBranch != nullptr )
	{
		AcmBranch *child = acm_get_first_child( pixelsBranch );
		for ( unsigned int i = 0; i < lightmapSize; ++i )
		{
			ApeLightmapPixel *pixel = &lightmap->pixels[ i ];

			assert( child != nullptr );
			for ( unsigned int j = 0; j < 3; ++j, child = acm_get_next_child( child ) )
			{
				// using var on stack here instead, as it resolves
				// warning: taking address of packed member of ‘struct ApeLightmapPixel’ may result in an unaligned pointer value
				_Float16 colour;
				acm_branch_get_float16( child, &colour );
				pixel->colour.v[ j ] = colour;
			}
		}
	}

	ape_lightmap_upload_( lightmap, edgeLength );

	return lightmap;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

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

static bool setup_face_lightmap( ApeRoom *room, ApeBrushFace *face )
{
	if ( !( ape_material_get_flags_( face->material ) & APE_MATERIAL_FLAG_LIGHTMAP ) )
	{
		return false;
	}

	// first calculate the bounds

	QmMathPlaneProjection projection = qm_math_plane_compute_projection( &( QmMathPlane ) {
	        .normal = face->normal,
	} );

	QmMathVector2f min = QM_MATH_VECTOR2F( FLT_MAX, FLT_MAX );
	QmMathVector2f max = QM_MATH_VECTOR2F( -FLT_MAX, -FLT_MAX );

	const ApeBrush *brush = face->parent;
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

	if ( face->lightmapLuxelDensity == 0 )
	{
		face->lightmapLuxelDensity = APE_BRUSH_FACE_LIGHTMAP_DEFAULT_LUXELS;
	}

	unsigned int w = ( unsigned int ) ceilf( ( max.x - min.x ) / face->lightmapLuxelDensity );
	unsigned int h = ( unsigned int ) ceilf( ( max.y - min.y ) / face->lightmapLuxelDensity );
	if ( w == 0 || h == 0 )
	{
		return false;
	}

	if ( w > room->lightmapEdgeLength || h > room->lightmapEdgeLength )
	{
		ape_console_warning_( "Encountered face too large to fit into lightmap, consider sub-dividing face!\n" );
		return false;
	}

	AuxTexturePackerNode *node = nullptr;
	for ( face->lightmapIndex = 0; face->lightmapIndex < APE_ROOM_MAX_LIGHTMAPS; ++face->lightmapIndex )
	{
		ApeLightmap *lightmap = room->lightmaps[ face->lightmapIndex ];
		if ( lightmap == nullptr )
		{
			if ( ( lightmap = ape_room_create_lightmap_( room ) ) == nullptr )
			{
				break;
			}

			//TODO: why are we storing ambient light as an rgba value!?
			lightmap_clear( lightmap, &QM_MATH_COLOUR4F_TO_3F( room->ambientLight ), room->lightmapEdgeLength );
		}

		if ( lightmap->packer == nullptr && ( lightmap->packer = aux_texture_packer_node_create_root( room->lightmapEdgeLength, room->lightmapEdgeLength ) ) == nullptr )
		{
			ape_console_warning_( "Failed to setup lightmap packer!\n" );
			return false;
		}

		node = aux_texture_packer_node_insert( lightmap->packer, w, h );
		if ( node != nullptr )
		{
			break;
		}
	}

	if ( node == nullptr )
	{
		ape_console_warning_( "Failed to add face to lightmap!\n" );
		return false;
	}

	// now update the uv so its correct relative to where it's going to be on the sheet

	ComMathRectI32 rect = aux_texture_packer_node_get_rect( node );

	face->lightmapArea.x = ( float ) rect.x / room->lightmapEdgeLength;
	face->lightmapArea.y = ( float ) rect.y / room->lightmapEdgeLength;
	face->lightmapArea.z = ( float ) ( rect.x + rect.w ) / room->lightmapEdgeLength;
	face->lightmapArea.w = ( float ) ( rect.y + rect.h ) / room->lightmapEdgeLength;

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

static void compute_face_lightmap( ApeRoom *room, const ApeBrushFace *face, ApeLight *light )
{
	if ( !( ape_material_get_flags_( face->material ) & APE_MATERIAL_FLAG_LIGHTMAP ) )
	{
		return;
	}

	if ( !ape_light_test_face( light, face ) )
	{
		return;
	}

	unsigned int w = ( face->lightmapArea.z - face->lightmapArea.x ) * room->lightmapEdgeLength;
	unsigned int h = ( face->lightmapArea.w - face->lightmapArea.y ) * room->lightmapEdgeLength;
	unsigned int x = face->lightmapArea.x * room->lightmapEdgeLength;
	unsigned int y = face->lightmapArea.y * room->lightmapEdgeLength;

	QmMathPlaneProjection projection = qm_math_plane_compute_projection( &( QmMathPlane ) {
	        .normal = face->normal,
	} );

	QmMathVector3f faceOrigin = face->bounds.absOrigin;
	faceOrigin                = qm_math_vector3f_sub( faceOrigin, qm_math_vector3f_scale_float( QM_MATH_PROJECTION_AXIS[ projection ][ 0 ], w / 2.0f * face->lightmapLuxelDensity ) );
	faceOrigin                = qm_math_vector3f_sub( faceOrigin, qm_math_vector3f_scale_float( QM_MATH_PROJECTION_AXIS[ projection ][ 1 ], h / 2.0f * face->lightmapLuxelDensity ) );

	float planeDistance = -qm_math_vector3f_dot_product( face->normal, face->bounds.absOrigin );

	for ( unsigned int row = 0; row < h; ++row )
	{
		for ( unsigned int col = 0; col < w; ++col )
		{
			// need to translate this now into a world coord relative to the origin of the face, and w / h ...
			// sooo uh, need to convert the lightmap area relative to the luxel size I guess?

			// this should return the x and y in world units
			float fx = col * face->lightmapLuxelDensity + face->lightmapLuxelDensity / 2.0f;
			float fy = row * face->lightmapLuxelDensity + face->lightmapLuxelDensity / 2.0f;

			QmMathVector3f luxelPos = faceOrigin;
			luxelPos                = qm_math_vector3f_add( luxelPos, qm_math_vector3f_scale_float( QM_MATH_PROJECTION_AXIS[ projection ][ 0 ], fx ) );
			luxelPos                = qm_math_vector3f_add( luxelPos, qm_math_vector3f_scale_float( QM_MATH_PROJECTION_AXIS[ projection ][ 1 ], fy ) );

			// reproject the luxel position back onto the face plane along the dropped axis
			float nDotD = qm_math_vector3f_dot_product( face->normal, QM_MATH_PROJECTION_NORMAL[ projection ] );
			if ( fabsf( nDotD ) > QM_MATH_EPSILON )
			{
				//TODO: hmmm we really should just have a 'plane' computed for a face on update,
				//		and use our 'plane_distance' method here instead, but that'll require some
				//		rework I can't be bothered with right now
				//		(also I'm not much of a math person but I can think of likely better way of doing this in future)
				float t  = -( qm_math_vector3f_dot_product( face->normal, luxelPos ) + planeDistance ) / nDotD;
				luxelPos = qm_math_vector3f_add( luxelPos, qm_math_vector3f_scale_float( QM_MATH_PROJECTION_NORMAL[ projection ], t ) );
			}

			QmMathVector3f lightPos = ape_light_get_position( light );
			QmMathVector3f lightDir;
			if ( light->type == APE_LIGHT_TYPE_SUN )
			{
				PLCollisionAABB bounds = ape_world_node_get_bounds( APE_WORLD_NODE( room ) );

				//TODO: this is unreliable, bounds will change at runtime - this should be reversed, cast from luxel out rather than casting from luxel to bounds...
				//		we're also seeing weird precision issues because of this at times, so, yeah...
				lightDir = ape_light_get_direction( light );
				lightPos = qm_math_vector3f_add( luxelPos, qm_math_vector3f_scale_float( qm_math_vector3f_invert( lightDir ), bounds.maxs.y * bounds.maxs.y ) );
			}
			else
			{
				lightDir = qm_math_vector3f_sub( luxelPos, lightPos );
				lightDir = qm_math_vector3f_normalize( lightDir );
			}

			QmMathColour4f lightColour = light->colour;

			ApeMaterial *material = face->material;
			if ( ape_material_can_receive_shadows( material ) && light->flags & APE_LIGHT_FLAG_SHADOWS )
			{
#if 0// penumbra - this should really just produce some sort of explicit sphere (but jittering works quite well anyway)

				float                         shadowFactor       = 0.0f;
				static constexpr unsigned int NUM_SHADOW_SAMPLES = 32;
				unsigned int                  seed               = NUM_SHADOW_SAMPLES;
				for ( unsigned int i = 0; i < NUM_SHADOW_SAMPLES; ++i )
				{
#	define JITTER_VARIATION ( qm_os_random_float( &seed, ( ( float ) i ) * ( NUM_SHADOW_SAMPLES * 0.5f ) / NUM_SHADOW_SAMPLES ) - \
		                       qm_os_random_float( &seed, ( ( float ) i ) * ( NUM_SHADOW_SAMPLES * 0.5f ) / NUM_SHADOW_SAMPLES ) )

					QmMathVector3f samplePos = lightPos;
					samplePos.x += JITTER_VARIATION;
					samplePos.y += JITTER_VARIATION;
					samplePos.z += JITTER_VARIATION;

					QmMathVector3f sampleDir = qm_math_vector3f_normalize( qm_math_vector3f_sub( luxelPos, samplePos ) );

					PLCollisionRay ray = {};
					ray.origin         = samplePos;
					ray.direction      = sampleDir;

					ApeCollisionIntersection result = {};
					if ( !ape_room_ray_intersect( room, &ray, &result ) || result.face == nullptr )
					{
						continue;
					}

					if ( result.face == face )
					{
						shadowFactor += 1.0f;
					}
				}

				shadowFactor /= NUM_SHADOW_SAMPLES;
				if ( shadowFactor <= 0.0f )
				{
					continue;
				}

				lightColour.r *= shadowFactor;
				lightColour.g *= shadowFactor;
				lightColour.b *= shadowFactor;

#else

				PLCollisionRay ray = {};
				ray.origin         = lightPos;
				ray.direction      = lightDir;

				ApeCollisionIntersection result = {};
				if ( !ape_room_ray_intersect( room, &ray, &result ) || result.face == nullptr )
				{
					//ape_draw_debug_line( lightPos, qm_math_vector3f_add( lightPos, qm_math_vector3f_scale_float( lightDir, 9999.0f ) ), PL_COLOUR_RED );
					continue;
				}

				//ape_draw_debug_line( lightPos, result.intersection, PL_COLOUR_GREEN );

				if ( result.face != face )
				{
					material = result.face->material;
					if ( !ape_material_is_blended( material ) && ape_material_can_cast_shadows( material ) )
					{
						continue;
					}

					ApeTexture *texture = ape_material_get_texture_( material, 0, "diffuseMap" );
					if ( texture != nullptr )
					{
#	if 0
						if ( texture->image != nullptr )
						{
							//TODO: riiiight, need to fetch the specific pixel we hit
						}
						else
#	endif
						{
							lightColour.r *= QM_MATH_BTOF( texture->average.r );
							lightColour.g *= QM_MATH_BTOF( texture->average.g );
							lightColour.b *= QM_MATH_BTOF( texture->average.b );
							//lightColour.r *= QM_MATH_BTOF( texture->average.r );
						}
					}

					//TODO: handle blended surfaces, refraction, yadda yadda
				}

#endif
			}


			// just pulled much of the below from our existing shaders...

			QmMathVector3f c;
			if ( light->type == APE_LIGHT_TYPE_SUN )
			{
				float l = QM_OS_MAX( qm_math_vector3f_dot_product( face->normal, lightDir ), 1.0f );
				c       = qm_math_vector3f_scale_float( qm_math_vector3f( lightColour.r, lightColour.g, lightColour.b ), l * lightColour.a );
			}
			else if ( light->type == APE_LIGHT_TYPE_SPOT )
			{
#if 0
				QmMathVector3f angles = ape_world_node_get_angles( APE_WORLD_NODE( light ) );
				PlAnglesAxes( angles, nullptr, nullptr, &lightDirection );
				lightDirection = qm_math_vector3f_normalize( lightDirection );

				float d = qm_math_vector3f_distance( lightPos, luxelPos );
				float theta = qm_math_vector3f_dot_product( lightDir, light->angle );
#endif
			}
			else// assumed omni
			{
				float d = qm_math_vector3f_distance( lightPos, luxelPos );
#ifdef APE_ENABLE_LIGHT_INV_SQUARE_FALLOFF
				float r = light->radius * 10.0f / ( d * d );
				float l = QM_OS_MAX( qm_math_vector3f_dot_product( face->normal, lightDir ), 1.0f );
				c = qm_math_vector3f_scale_float( qm_math_vector3f( lightColour.r, lightColour.g, lightColour.b ), l );
#else
				float r = QM_MATH_CLAMP( 0.0f, 1.0f - d / light->radius, 1.0f );
				float l = QM_OS_MAX( qm_math_vector3f_dot_product( face->normal, lightDir ), 1.0f );
				c = qm_math_vector3f_scale_float( qm_math_vector3f( lightColour.r, lightColour.g, lightColour.b ), l * lightColour.a );
#endif

				c = qm_math_vector3f_scale_float( c, r );
			}

			ApeLightmap *lightmap = room->lightmaps[ face->lightmapIndex ];
			assert( lightmap != nullptr );

			ApeLightmapPixel *pixel = &lightmap->pixels[ ( y + row ) * room->lightmapEdgeLength + ( x + col ) ];
			pixel->colour.r += c.x;
			pixel->colour.g += c.y;
			pixel->colour.b += c.z;
		}
	}
}

static void gather_nodes( ApeWorldNode *node, QmOsLinkedList *lights, QmOsLinkedList *faces )
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
		ApeBrush *brush = ( ApeBrush * ) node;
		for ( unsigned int i = 0; i < brush->numFaces; ++i )
		{
			ApeBrushFace *face = &brush->faces[ i ];
			if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN )
			{
				face->lightmapIndex = APE_BRUSH_FACE_LIGHTMAP_INVALID;
				continue;
			}

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

			// add it to a list so we can quickly iterate over it later
			qm_os_linked_list_push_back( faces, face );

			// mark it dirty so we reupload later with the new uv
			ape_brush_mark_parent_dirty( brush );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		gather_nodes( child, lights, faces );
	}
}

void ape_editor_light_generate_( ApeRoom *room )
{
	ape_console_print_( "Generating lightmap...\n" );

	// first, gather all the objects for the given room we need to operate on

	QmOsLinkedList *lights = qm_os_linked_list_create();
	QmOsLinkedList *faces  = qm_os_linked_list_create();
	if ( lights == nullptr || faces == nullptr )
	{
		ape_console_warning_( "Failed to create lists for lightmap generation!\n" );
		goto cleanup;
	}

	double startTime = qm_os_time_get_seconds();

	gather_nodes( APE_WORLD_NODE( room ), lights, faces );

	unsigned int numLights = qm_os_linked_list_get_size( lights );
	unsigned int numFaces  = qm_os_linked_list_get_size( faces );
	if ( numLights == 0 || numFaces == 0 )
	{
		ape_console_warning_( "No faces or lights to operate on for lightmap!\n" );
		goto cleanup;
	}

	ape_console_print_( "Processing %u lights, %u faces...\n",
	                    qm_os_linked_list_get_size( lights ),
	                    qm_os_linked_list_get_size( faces ) );

	//TODO: this should only be done if it's dirty!
	ape_room_destroy_lightmaps_( room );

	ApeBrushFace *face;
	QM_OS_LINKED_LIST_ITERATE( face, faces, i )
	{
		if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN || !setup_face_lightmap( room, face ) )
		{
			face->lightmapIndex = APE_BRUSH_FACE_LIGHTMAP_INVALID;
			continue;
		}

		//TODO: sort faces, then set them up for better packing

		// now, generate the lightmap for each light
		ApeLight *light;
		QM_OS_LINKED_LIST_ITERATE( light, lights, i )
		{
			compute_face_lightmap( room, face, light );
		}
	}

	// convert the lightmap into a texture we can use
	ape_room_upload_lightmaps_( room );

	double endTime = qm_os_time_get_seconds();
	ape_console_print_( "Lightmap generation took %.3f seconds (%u lightmaps created).\n", endTime - startTime, room->numLightmaps );

cleanup:
	qm_os_memory_free( lights );
	qm_os_memory_free( faces );

	// the packers are only needed for lightmap generation,
	// so we can trash them now
	for ( unsigned int i = 0; i < room->numLightmaps; ++i )
	{
		ApeLightmap *lightmap = room->lightmaps[ i ];
		if ( lightmap->packer == nullptr )
		{
			continue;
		}

		qm_os_memory_free( lightmap->packer );
		lightmap->packer = nullptr;
	}
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

static QmMathVector2f uvOffset;
static unsigned int   lightmapIndex;

static bool display_brush_uv( ApeWorldNode *node, void *user )
{
	const float edgeLength = *( float * ) user;

	ApeBrush *brush = ( ApeBrush * ) node;
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		ApeBrushFace *face = &brush->faces[ i ];
		if ( face->lightmapIndex != lightmapIndex )
		{
			continue;
		}

		unsigned int seed = ( uintptr_t ) face;

		QmMathColour4ub colour;
		colour.r = 128 + qm_os_random_int( &seed ) % 128 - 1;
		colour.g = 128 + qm_os_random_int( &seed ) % 128 - 1;
		colour.b = 128 + qm_os_random_int( &seed ) % 128 - 1;
		colour.a = 255;

		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			unsigned int k = ( j + 1 ) % face->numVertices;

			QmMathVector2f start = qm_math_vector2f_add( uvOffset, face->vertices[ face->edgeLoopOrder[ j ] ].lightmapCoords );
			QmMathVector2f end   = qm_math_vector2f_add( uvOffset, face->vertices[ face->edgeLoopOrder[ k ] ].lightmapCoords );

			start = qm_math_vector2f_scale_float( start, edgeLength );
			end   = qm_math_vector2f_scale_float( end, edgeLength );

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
	if ( room == nullptr || room->numLightmaps == 0 )
	{
		return;
	}

	ApeMaterial *debugLightmapMaterial = ape_material_cache( "materials/debug/debug_lightmap.mat.n", APE_CACHE_GROUP_GLOBAL, false );
	if ( debugLightmapMaterial == nullptr )
	{
		return;
	}

	static constexpr float SCALE      = 2.0f;
	float                  edgeLength = ( float ) room->lightmapEdgeLength * SCALE;

	uvOffset = QM_MATH_VECTOR2F_ZERO;

	for ( unsigned int i = 0; i < room->numLightmaps; ++i, uvOffset.x += edgeLength )
	{
		lightmapIndex         = i;
		ApeLightmap *lightmap = room->lightmaps[ i ];

		ape_rendererState_.lightmapTexture = lightmap->texture->internal;
		ape_draw_textured_quad( debugLightmapMaterial, uvOffset.x, uvOffset.y, edgeLength, edgeLength, &PL_COLOUR_WHITE, 0.0f );
		ape_rendererState_.lightmapTexture = nullptr;

		ape_world_node_visit_children( APE_WORLD_NODE( room ), APE_WORLD_NODE_TYPE_BRUSH, true, display_brush_uv, &edgeLength );
	}

	ape_material_release( debugLightmapMaterial );
}