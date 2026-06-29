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
// Light Grid
// The light grid allows objects in the world to more accurately sample lighting data
// without depending on the lightmap. So, essentially they can be lit somewhat
// more precisely in 3D space.
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeLightGridCell
{
	QmMathColour3f16 totalLight;
	QmMathVector3f   averageDir;
	uint8_t          numLights;
} ApeLightGridCell;

typedef struct ApeLightGrid
{
	QmMathVector3f mins;
	QmMathVector3f maxs;
	QmMathVector3i density;
	QmMathVector3f cellSize;

	ApeLightGridCell *cells;
	unsigned int      numCells;
} ApeLightGrid;

static QmMathVector3f light_grid_get_cell_world_position( const ApeLightGrid *self, const QmMathVector3i xyz )
{
	return QM_MATH_VECTOR3F( self->mins.x + ( xyz.x + 0.5f ) * self->cellSize.x,
	                         self->mins.y + ( xyz.y + 0.5f ) * self->cellSize.y,
	                         self->mins.z + ( xyz.z + 0.5f ) * self->cellSize.z );
}

static ApeLightGridCell *light_grid_get_cell( const ApeLightGrid *self, const QmMathVector3i xyz )
{
	return &self->cells[ xyz.x + xyz.y * self->density.x + xyz.z * self->density.x * self->density.y ];
}

static ApeLightGridCell *light_grid_get_cell_by_position( const ApeLightGrid *self, const QmMathVector3f position )
{
	if ( position.x < self->mins.x || position.y < self->mins.y || position.z < self->mins.z ||
	     position.x > self->maxs.x || position.y > self->maxs.y || position.z > self->maxs.z )
	{
		return nullptr;
	}

	QmMathVector3f grid = qm_math_vector3f_div( qm_math_vector3f_sub( position, self->mins ), self->cellSize );

	return light_grid_get_cell( self, QM_MATH_VECTOR3I( grid.x, grid.y, grid.z ) );
}

static void light_grid_destroy( void *ptr )
{
	ApeLightGrid *grid = ptr;

	qm_os_memory_free( grid->cells );
}

ApeLightGrid *ape_light_grid_create_( const QmMathVector3f mins, const QmMathVector3f maxs, const QmMathVector3i density )
{
	ApeLightGrid *grid = ape_memory_alloc( sizeof( ApeLightGrid ), light_grid_destroy, false );
	if ( grid == nullptr )
	{
		return nullptr;
	}

	grid->mins = mins;
	grid->maxs = maxs;

	grid->density = density;

	grid->cellSize = qm_math_vector3f_div( qm_math_vector3f_sub( grid->maxs, grid->mins ),
	                                       QM_MATH_VECTOR3F( density.x, density.y, density.z ) );

	grid->numCells = grid->density.x * grid->density.y * grid->density.z;
	grid->cells    = ape_memory_calloc( grid->numCells, sizeof( ApeLightGridCell ), nullptr, false );
	if ( grid->cells == nullptr )
	{
		qm_os_memory_free( grid );
		grid = nullptr;
	}

	return grid;
}

void ape_light_grid_compute_( ApeLightGrid *self, ApeRoom *room, ApeLight **lights, unsigned int numLights )
{
	QmMathVector3f cellMins = qm_math_vector3f_invert( self->cellSize );
	QmMathVector3f cellMaxs = self->cellSize;

	for ( unsigned int z = 0; z < self->density.z; ++z )
	{
		for ( unsigned int y = 0; y < self->density.y; ++y )
		{
			for ( unsigned int x = 0; x < self->density.x; ++x )
			{
				ApeLightGridCell *cell = light_grid_get_cell( self, QM_MATH_VECTOR3I( x, y, z ) );
				if ( cell->numLights >= UINT8_MAX )
				{
					ape_console_warning_( "Hit maximum light limit for cell, skipping!\n" );
					continue;
				}

				QmMathVector3f worldPos = light_grid_get_cell_world_position( self, QM_MATH_VECTOR3I( x, y, z ) );
				for ( unsigned int i = 0; i < numLights; ++i )
				{
					const ApeLight *light = lights[ i ];
					if ( !ape_light_test_bounds( light, worldPos, cellMins, cellMaxs ) )
					{
						continue;
					}

					QmMathVector3f lightPos = ape_light_get_position( light );
					QmMathVector3f lightDir;
					if ( light->type == APE_LIGHT_TYPE_SUN )
					{
						PLCollisionAABB bounds = ape_world_node_get_bounds( APE_WORLD_NODE( room ) );

						//TODO: this is unreliable, bounds will change at runtime - this should be reversed, cast from luxel out rather than casting from luxel to bounds...
						//		we're also seeing weird precision issues because of this at times, so, yeah...
						lightDir = ape_light_get_direction( light );
						lightPos = qm_math_vector3f_add( worldPos, qm_math_vector3f_scale_float( qm_math_vector3f_invert( lightDir ), bounds.maxs.y * bounds.maxs.y ) );
					}
					else
					{
						lightDir = qm_math_vector3f_sub( worldPos, lightPos );
						lightDir = qm_math_vector3f_normalize( lightDir );
					}

					PLCollisionRay ray = {};
					ray.origin         = lightPos;
					ray.direction      = lightDir;

					ApeCollisionIntersection result = {};
					if ( ape_room_ray_intersect( room, &ray, &result ) )
					{
						float lightDistance = qm_math_vector3f_distance( worldPos, lightPos );
						if ( result.distance < lightDistance )
						{
							continue;
						}
					}

					QmMathVector3f c = qm_math_vector3f_scale_float( qm_math_vector3f( light->colour.r, light->colour.g, light->colour.b ), light->colour.a );
					if ( light->type == APE_LIGHT_TYPE_SPOT )
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
						float d = qm_math_vector3f_distance( lightPos, worldPos );
#ifdef APE_ENABLE_LIGHT_INV_SQUARE_FALLOFF
						float r = light->radius * 10.0f / ( d * d );
#else
						float r = QM_MATH_CLAMP( 0.0f, 1.0f - d / light->radius, 1.0f );
#endif
						c = qm_math_vector3f_scale_float( c, r );
					}

					cell->totalLight.r += c.x;
					cell->totalLight.g += c.y;
					cell->totalLight.b += c.z;
					cell->numLights++;
				}
			}
		}
	}
}

const ApeLightGridCell *ape_light_grid_sample_cell_( const ApeLightGrid *self, const QmMathVector3f position, QmMathColour3f16 *dstColour, QmMathVector3f *dstDir )
{
	ApeLightGridCell *cell = light_grid_get_cell_by_position( self, position );
	if ( cell != nullptr )
	{
		dstColour->r = cell->totalLight.r / ( cell->numLights + 1 );
		dstColour->g = cell->totalLight.g / ( cell->numLights + 1 );
		dstColour->b = cell->totalLight.b / ( cell->numLights + 1 );
	}

	return cell;
}

void ape_light_grid_draw_( const ApeLightGrid *self )
{
	PLCollisionAABB bounds = {};
	bounds.mins.x          = -self->cellSize.x;
	bounds.mins.y          = -self->cellSize.y;
	bounds.mins.z          = -self->cellSize.z;
	bounds.maxs.x          = self->cellSize.x;
	bounds.maxs.y          = self->cellSize.y;
	bounds.maxs.z          = self->cellSize.z;

	for ( unsigned int z = 0; z < self->density.z; ++z )
	{
		for ( unsigned int y = 0; y < self->density.y; ++y )
		{
			for ( unsigned int x = 0; x < self->density.x; ++x )
			{
				ApeLightGridCell *cell = light_grid_get_cell( self, QM_MATH_VECTOR3I( x, y, z ) );

				bounds.origin = light_grid_get_cell_world_position( self, QM_MATH_VECTOR3I( x, y, z ) );

				QmMathColour3f colour = {};
				if ( cell->numLights > 0 )
				{
					colour.r = QM_MATH_CLAMP( 0.0f, cell->totalLight.r / cell->numLights, 1.0f );
					colour.g = QM_MATH_CLAMP( 0.0f, cell->totalLight.g / cell->numLights, 1.0f );
					colour.b = QM_MATH_CLAMP( 0.0f, cell->totalLight.b / cell->numLights, 1.0f );
				}

				ape_draw_debug_aabb( &bounds, QM_MATH_COLOUR3F_TO_4UB( colour, 255 ) );
			}
		}
	}
}

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
	ape_texture_release_reference( self->texture );
	ape_memory_flush_unreferenced_resources();

	qm_os_memory_free( self->pixels );
	qm_os_memory_free( self->packer );
	qm_os_memory_free( self );
}

void ape_lightmap_upload_( ApeLightmap *self, unsigned int edgeLength )
{
	if ( self->texture != nullptr )
	{
		ape_texture_release_reference( self->texture );
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

static void get_face_bounds( const ApeBrushFace *face, QmMathVector2f *minDst, QmMathVector2f *maxDst )
{
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

	if ( minDst != nullptr )
	{
		*minDst = min;
	}
	if ( maxDst != nullptr )
	{
		*maxDst = max;
	}
}

static bool setup_face_lightmap( ApeRoom *room, ApeBrushFace *face )
{
	// first calculate the bounds

	QmMathPlaneProjection projection = qm_math_plane_compute_projection( &( QmMathPlane ) {
	        .normal = face->normal,
	} );

	QmMathVector2f min, max;
	get_face_bounds( face, &min, &max );

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

	if ( ape_brush_face_is_emissive( face ) )
	{
		// emissive faces don't actually use the lightmaps
		face->lightmapArea.x = 0.0f;
		face->lightmapArea.y = 0.0f;
		face->lightmapArea.z = ( float ) w;
		face->lightmapArea.w = ( float ) h;
		return true;
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

			room->lightmaps[ face->lightmapIndex ] = lightmap;
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

	AuxMathRectI32 rect = aux_texture_packer_node_get_rect( node );

	face->lightmapArea.x = ( float ) rect.x / room->lightmapEdgeLength;
	face->lightmapArea.y = ( float ) rect.y / room->lightmapEdgeLength;
	face->lightmapArea.z = ( float ) ( rect.x + rect.w ) / room->lightmapEdgeLength;
	face->lightmapArea.w = ( float ) ( rect.y + rect.h ) / room->lightmapEdgeLength;

	//TODO: this should be relative to luxel-size etc., blergh...
	static constexpr float PADDING = 0.002f;

	const ApeBrush *brush = face->parent;
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

static void compute_face_vertex( ApeRoom *room, ApeBrushFace *face, ApeLight *light )
{
	// ooh baby, this is so much easier than the lightmaps :)

	QmMathVector3f lightPos = ape_light_get_position( light );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		ApeBrushFaceVertex *vertex = &face->vertices[ i ];

		vertex->colour = ( QmMathColour4f ) { .a = 1.0f };

		// need to transform each vertex
		ApeBrush      *brush     = face->parent;
		PLMatrix4      transform = ape_world_node_get_transform( APE_WORLD_NODE( brush ) );
		QmMathVector3f vertexPos = brush->vertices[ vertex->posIndex ];
		vertexPos                = PlTransformVector3( &vertexPos, &transform );

		QmMathVector3f lightDir;
		if ( light->type == APE_LIGHT_TYPE_SUN )
		{
			PLCollisionAABB bounds = ape_world_node_get_bounds( APE_WORLD_NODE( room ) );

			//TODO: this is unreliable, bounds will change at runtime - this should be reversed, cast from luxel out rather than casting from luxel to bounds...
			//		we're also seeing weird precision issues because of this at times, so, yeah...
			lightDir = ape_light_get_direction( light );
			lightPos = qm_math_vector3f_add( vertexPos, qm_math_vector3f_scale_float( qm_math_vector3f_invert( lightDir ), bounds.maxs.y * bounds.maxs.y ) );
		}
		else
		{
			lightDir = qm_math_vector3f_sub( vertexPos, lightPos );
			lightDir = qm_math_vector3f_normalize( lightDir );
		}

		QmMathColour4f lightColour = light->colour;

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
				if ( !ape_material_is_blended( material ) && ape_material_can_cast_shadows( material ) )
				{
					continue;
				}
			}
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
			float d = qm_math_vector3f_distance( lightPos, vertexPos );
#ifdef APE_ENABLE_LIGHT_INV_SQUARE_FALLOFF
			float r = light->radius * 10.0f / ( d * d );
			float l = QM_OS_MAX( qm_math_vector3f_dot_product( face->normal, lightDir ), 1.0f );
			c       = qm_math_vector3f_scale_float( qm_math_vector3f( lightColour.r, lightColour.g, lightColour.b ), l );
#else
			float r = QM_MATH_CLAMP( 0.0f, 1.0f - d / light->radius, 1.0f );
			float l = QM_OS_MAX( qm_math_vector3f_dot_product( face->normal, lightDir ), 1.0f );
			c       = qm_math_vector3f_scale_float( qm_math_vector3f( lightColour.r, lightColour.g, lightColour.b ), l * lightColour.a );
#endif

			c = qm_math_vector3f_scale_float( c, r );
		}

		vertex->colour.r += c.x;
		vertex->colour.g += c.y;
		vertex->colour.b += c.z;
		ape_console_print_( "%f %f %f\n", vertex->colour.r, vertex->colour.g, vertex->colour.b );
	}
}

static void compute_face_lightmap( ApeRoom *room, const ApeBrushFace *face, ApeLight *light )
{
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

#if 0// dumb dumb dumb
			if ( ape_brush_face_is_emissive( face ) )
			{
				// emissive faces don't actually use the lightmaps
				QmMathColour4f colour     = ape_brush_face_get_emission( face );
				ApeLight      *luxelLight = ape_create_light( APE_WORLD_NODE( room ), &luxelPos, &colour, 64.0f, APE_LIGHT_TYPE_OMNI, 0 );
				currentFace = face;
				compute_light( luxelLight, room, faces, numFaces );
				currentFace = nullptr;
				ape_world_node_destroy( APE_WORLD_NODE( luxelLight ) );
				continue;
			}
#endif

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
				c       = qm_math_vector3f_scale_float( qm_math_vector3f( lightColour.r, lightColour.g, lightColour.b ), l );
#else
				float r = QM_MATH_CLAMP( 0.0f, 1.0f - d / light->radius, 1.0f );
				float l = QM_OS_MAX( qm_math_vector3f_dot_product( face->normal, lightDir ), 1.0f );
				c       = qm_math_vector3f_scale_float( qm_math_vector3f( lightColour.r, lightColour.g, lightColour.b ), l * lightColour.a );
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

static void gather_nodes( ApeWorldNode *node, PLVectorArray *lights, PLVectorArray *faces, bool buildLightmap )
{
	if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
	{
		ApeLight *light = ( ApeLight * ) node;
		if ( !( light->flags & APE_LIGHT_FLAG_DYNAMIC ) )
		{
			PlPushBackVectorArrayElement( lights, light );
		}
	}
	//TODO: move all this out of gather and into the lightmap compute
	else if ( node->type == APE_WORLD_NODE_TYPE_BRUSH && buildLightmap )
	{
		ApeBrush *brush = ( ApeBrush * ) node;
		ApeRoom  *room  = ape_world_node_get_room( APE_WORLD_NODE( brush ) );
		for ( unsigned int i = 0; i < brush->numFaces; ++i )
		{
			ApeBrushFace *face = &brush->faces[ i ];

			// clear the lightmap index
			face->lightmapIndex = APE_BRUSH_FACE_LIGHTMAP_INVALID;

			if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN || brush->lightingType == APE_BRUSH_LIGHTING_TYPE_VERTEX ||
			     ( !ape_brush_face_is_emissive( face ) && !( ape_material_get_flags_( face->material ) & APE_MATERIAL_FLAG_LIGHTMAP ) ) )
			{
				continue;
			}

			if ( !setup_face_lightmap( room, face ) )
			{
				ape_console_warning_( "Failed to setup lightmap for face!\n" );
				continue;
			}

			// if we're dealing with a portal, we want to navigate down
			// to figure out what else we need to deal with
			if ( ape_brush_face_is_portal( face ) )
			{
				ApeBrushFace *dstFace = ape_brush_face_get_portal_destination( face );
				if ( dstFace != face )
				{
					//TODO: navigate through portals, add to list, handle recursion, wheeee
					continue;
				}

				if ( !ape_brush_face_is_mirror( face ) )
				{
					continue;
				}
			}

			// add it to a list so we can quickly iterate over it later
			PlPushBackVectorArrayElement( faces, face );

			// mark it dirty so we reupload later with the new uv
			ape_brush_mark_parent_dirty( brush );
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		gather_nodes( child, lights, faces, buildLightmap );
	}
}

static void compute_light( ApeLight *light, ApeRoom *room, ApeBrushFace **faces, const unsigned int numFaces )
{
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		ApeBrush *brush = faces[ i ]->parent;

		if ( !ape_light_test_face( light, faces[ i ] ) )
		{
			continue;
		}

		if ( brush->lightingType == APE_BRUSH_LIGHTING_TYPE_VERTEX )
		{
			compute_face_vertex( room, faces[ i ], light );
		}
		else
		{
			compute_face_lightmap( room, faces[ i ], light );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// General "Light" API
/////////////////////////////////////////////////////////////////////////////////////

static void room_compute_light_grid( ApeRoom *self, ApeLight **lights, unsigned int numLights )
{
	qm_os_memory_free( self->lightGrid );

	PLCollisionAABB bounds = ape_world_node_get_bounds( APE_WORLD_NODE( self ) );
	self->lightGrid        = ape_light_grid_create_( bounds.mins, bounds.maxs, QM_MATH_VECTOR3I( 32, 32, 32 ) );
	if ( self->lightGrid != nullptr )
	{
		double gridStart = qm_os_time_get_seconds();

		ape_light_grid_compute_( self->lightGrid, self, lights, numLights );

		double gridEnd = qm_os_time_get_seconds();
		ape_console_print_( "Light grid generation took %.3f seconds.\n", gridEnd - gridStart );
	}
	else
	{
		ape_console_warning_( "Failed to create light grid, room will be lit incorrectly!\n" );
	}
}

static void room_compute_lightmap( ApeRoom *self, ApeLight **lights, unsigned int numLights, ApeBrushFace **faces, unsigned int numFaces )
{
	double startTime = qm_os_time_get_seconds();

	// now, generate the lightmap for each light
	for ( unsigned int i = 0; i < numLights; ++i )
	{
		compute_light( lights[ i ], self, faces, numFaces );
	}

	double endTime = qm_os_time_get_seconds();
	ape_console_print_( "Lightmap generation took %.3f seconds (%u lightmaps created).\n", endTime - startTime, self->numLightmaps );

#if 0
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		if ( !ape_brush_face_is_emissive( faces[ i ] ) )
		{
			continue;
		}

		compute_face_lightmap( room, faces[ i ], nullptr );
	}
#endif

	// convert the lightmap into a texture we can use
	ape_room_upload_lightmaps_( self );
}

void ape_editor_light_generate_( ApeRoom *room, bool buildLightmap, bool buildLightGrid )
{
	// first, gather all the objects for the given room we need to operate on

	PLVectorArray *lightsArray = PlCreateVectorArray( 1024 );
	PLVectorArray *facesArray  = PlCreateVectorArray( 2048 );
	if ( lightsArray == nullptr || facesArray == nullptr )
	{
		ape_console_warning_( "Failed to create lists for lightmap generation!\n" );
		goto cleanup;
	}

	if ( buildLightmap )
	{
		//TODO: this should only be done if it's dirty!
		ape_room_destroy_lightmaps_( room );
	}

	gather_nodes( APE_WORLD_NODE( room ), lightsArray, facesArray, buildLightmap );

	unsigned int numLights;
	ApeLight   **lights = ( ApeLight ** ) PlGetVectorArrayDataEx( lightsArray, &numLights );

	unsigned int   numFaces;
	ApeBrushFace **faces = ( ApeBrushFace ** ) PlGetVectorArrayDataEx( facesArray, &numFaces );

	ape_console_print_( "Processing %u lights, %u faces...\n", numLights, numFaces );

	if ( buildLightGrid )
	{
		room_compute_light_grid( room, lights, numLights );
	}

	if ( buildLightmap )
	{
		room_compute_lightmap( room, lights, numLights, faces, numFaces );
	}

cleanup:
	PlDestroyVectorArray( lightsArray );
	PlDestroyVectorArray( facesArray );

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

	ape_editor_light_generate_( room, true, true );
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

	ape_material_release_reference( debugLightmapMaterial );
}