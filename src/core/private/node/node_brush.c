// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"
#include "qmos/public/qm_os_shared_ptr.h"

#include "ape_private.h"
#include "renderer/material/material.h"
#include "world/world.h"

static void clear_tagged_surfaces( const ApeBrush *self, ApeRoom *room )
{
	// remove all of the faces from the lookup!
	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		ApeBrushFace *face = &self->faces[ i ];
		if ( *face->tag == '\0' )
		{
			continue;
		}

		ape_room_remove_tagged_surface( room, face );
	}
}

static void add_tagged_surfaces( const ApeBrush *self, ApeRoom *room )
{
	// add all of the faces from the lookup!
	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		ApeBrushFace *face = &self->faces[ i ];
		if ( *face->tag == '\0' )
		{
			continue;
		}

		ape_room_add_tagged_surface( room, face );
	}
}

static void *create_brush( ApeWorldNode *parent )
{
	ApeBrush *brush = QM_OS_MEMORY_NEW( ApeBrush );
	ape_world_node_setup_( &brush->base, parent, APE_WORLD_NODE_TYPE_BRUSH, nullptr, &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO );
	return brush;
}

ApeBrush *ape_brush_create( ApeWorldNode *parent, const char *name, const QmMathVector3f *position, const QmMathVector3f *angles )
{
	ApeBrush *brush = create_brush( parent );

	ape_world_node_set_name( APE_WORLD_NODE( brush ), name );
	ape_world_node_set_position( APE_WORLD_NODE( brush ), position );
	ape_world_node_set_angles( APE_WORLD_NODE( brush ), angles );

	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}

	return brush;
}

static void destroy_brush( void *data, ApeWorldNode *parent )
{
	ApeBrush *self = data;
	if ( self == nullptr )
	{
		return;
	}

	//HACK: notify the room it's rebuild time!
	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}

	ApeRoom *room = ape_world_node_get_room( parent );
	if ( room != nullptr )
	{
		clear_tagged_surfaces( self, room );
	}

	qm_os_memory_free( self->vertices );

	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		if ( self->faces[ i ].ptr == nullptr )
		{
			continue;
		}

		qm_os_shared_ptr_release( self->faces[ i ].ptr );
		qm_os_shared_ptr_set( self->faces[ i ].ptr, nullptr );
		self->faces[ i ].ptr = nullptr;
	}
	qm_os_memory_free( self->faces );

#if defined( APE_SUPPORT_EDITOR )
	qm_os_memory_free( self->vertexSelectColours );
#endif

	qm_os_memory_free( self );
}

static ApeWorldNode *clone_brush( ApeWorldNode *src )
{
	ApeBrush *srcBrush = ( ApeBrush * ) src;
	ApeBrush *dstBrush = ape_brush_create( src->parent, src->name, &src->position, &src->angles );
	if ( dstBrush == nullptr )
	{
		ape_console_warning_( "Failed to create brush for duplication!\n" );
		return nullptr;
	}

	dstBrush->type = srcBrush->type;

	dstBrush->numVertices = srcBrush->numVertices;
	dstBrush->vertices    = QM_OS_MEMORY_NEW_( QmMathVector3f, dstBrush->numVertices );
	for ( unsigned int j = 0; j < dstBrush->numVertices; ++j )
	{
		dstBrush->vertices[ j ] = srcBrush->vertices[ j ];
	}

	dstBrush->numFaces = srcBrush->numFaces;
	dstBrush->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, dstBrush->numFaces );
	for ( unsigned int j = 0; j < dstBrush->numFaces; ++j )
	{
		ape_brush_face_setup( &dstBrush->faces[ j ] );

		//TODO: materials are an annoying pain in the ass because of how we're handling references... this should be fixed...
		const char  *materialPath     = ape_material_get_path( srcBrush->faces[ j ].material );
		ApeMaterial *material         = ape_material_cache( materialPath, APE_CACHE_GROUP_WORLD, true );
		dstBrush->faces[ j ].material = material;

		dstBrush->faces[ j ].materialScale  = srcBrush->faces[ j ].materialScale;
		dstBrush->faces[ j ].materialOffset = srcBrush->faces[ j ].materialOffset;
		dstBrush->faces[ j ].materialAngle  = srcBrush->faces[ j ].materialAngle;

		dstBrush->faces[ j ].lightmapArea         = srcBrush->faces[ j ].lightmapArea;
		dstBrush->faces[ j ].lightmapIndex        = srcBrush->faces[ j ].lightmapIndex;
		dstBrush->faces[ j ].lightmapLuxelDensity = srcBrush->faces[ j ].lightmapLuxelDensity;

		dstBrush->faces[ j ].normal = srcBrush->faces[ j ].normal;

		dstBrush->faces[ j ].flags = srcBrush->faces[ j ].flags;

		dstBrush->faces[ j ].bounds      = srcBrush->faces[ j ].bounds;
		dstBrush->faces[ j ].numVertices = srcBrush->faces[ j ].numVertices;
		for ( unsigned int k = 0; k < dstBrush->faces[ j ].numVertices; ++k )
		{
			dstBrush->faces[ j ].vertices[ k ].posIndex      = srcBrush->faces[ j ].vertices[ k ].posIndex;
			dstBrush->faces[ j ].vertices[ k ].textureCoords = srcBrush->faces[ j ].vertices[ k ].textureCoords;
			dstBrush->faces[ j ].vertices[ k ].normal        = srcBrush->faces[ j ].vertices[ k ].normal;
			dstBrush->faces[ j ].edgeLoopOrder[ k ]          = srcBrush->faces[ j ].edgeLoopOrder[ k ];
		}

		dstBrush->faces[ j ].tangent   = srcBrush->faces[ j ].tangent;
		dstBrush->faces[ j ].bitangent = srcBrush->faces[ j ].bitangent;

		strcpy( dstBrush->faces[ j ].destinationTag, srcBrush->faces[ j ].destinationTag );

		dstBrush->faces[ j ].parent = dstBrush;
	}

	ape_brush_compute_bounds( dstBrush );

	return APE_WORLD_NODE( dstBrush );
}

#if 0//unused
static unsigned int convert_brush_polygon_to_triangles( const ApeBrushFace *face, unsigned int *indices )
{
	assert( face->numVertices >= 3 );

	unsigned int  numTriangles = 0;
	unsigned int *index        = indices;

#	if 0// concave polygon

	/**
	 * Here's an algorithm I think could work - two passes...
	 * 	1. Follow edge loop as we do for convex, but if there is an overlap, skip and mark
	 * 	2. Now continue on from those we skipped in a similar way to the above, with the first skipped being the start, if there is another overlap then repeat for those
	 *
	 * Math isn't my strong point, so it's probably dumb.
	 */

#	else// convex polygon

	for ( unsigned int i = 1; i + 1 < face->numVertices; ++i )
	{
		index[ 0 ] = ( face->edgeLoop[ 0 ] - face->vertices );
		index[ 1 ] = ( face->edgeLoop[ i ] - face->vertices );
		index[ 2 ] = ( face->edgeLoop[ i + 1 ] - face->vertices );
		index += 3;

		numTriangles++;
	}

#	endif

	return numTriangles;
}
#endif

void ape_brush_face_compute_normal( ApeBrushFace *face )
{
	face->normal = qm_math_vector3f( 0.0f, 0.0f, 0.0f );

	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	assert( face->numVertices >= 3 );
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		unsigned int j = ( i + 1 ) % face->numVertices;// next vertex index (wraps around)

		const QmMathVector3f *current = &brush->vertices[ face->vertices[ face->edgeLoopOrder[ i ] ].posIndex ];
		const QmMathVector3f *next    = &brush->vertices[ face->vertices[ face->edgeLoopOrder[ j ] ].posIndex ];
		const QmMathVector3f *prev    = &brush->vertices[ face->vertices[ face->edgeLoopOrder[ i == 0 ? face->numVertices - 1 : i - 1 ] ].posIndex ];

		QmMathVector3f edge1 = qm_math_vector3f( next->x - current->x, next->y - current->y, next->z - current->z );
		QmMathVector3f edge2 = qm_math_vector3f( prev->x - current->x, prev->y - current->y, prev->z - current->z );

		QmMathVector3f n = qm_math_vector3f_cross_product( edge1, edge2 );
		face->normal     = qm_math_vector3f_add( face->normal, n );
	}

	face->normal = qm_math_vector3f_normalize( face->normal );
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		face->vertices[ i ].normal = face->normal;
	}
}

static void compute_brush_face_tangents( ApeBrushFace *face )
{
	face->tangent = face->bitangent = ( QmMathVector3f ) {};

	assert( face->numVertices >= 3 );

#if 1

	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	ApeBrushFaceVertex *v0 = &face->vertices[ face->edgeLoopOrder[ 0 ] ];
	ApeBrushFaceVertex *v1 = &face->vertices[ face->edgeLoopOrder[ 1 ] ];
	ApeBrushFaceVertex *v2 = &face->vertices[ face->edgeLoopOrder[ 2 ] ];

	QmMathVector3f edge1 = qm_math_vector3f_sub( brush->vertices[ v1->posIndex ], brush->vertices[ v0->posIndex ] );
	QmMathVector3f edge2 = qm_math_vector3f_sub( brush->vertices[ v2->posIndex ], brush->vertices[ v0->posIndex ] );

	QmMathVector2f deltaUV1 = qm_math_vector2f_sub( v1->textureCoords, v0->textureCoords );
	QmMathVector2f deltaUV2 = qm_math_vector2f_sub( v2->textureCoords, v0->textureCoords );

	float f = 1.0f / ( deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y );

	face->tangent.x = f * ( deltaUV2.y * edge1.x - deltaUV1.y * edge2.x );
	face->tangent.y = f * ( deltaUV2.y * edge1.y - deltaUV1.y * edge2.y );
	face->tangent.z = f * ( deltaUV2.y * edge1.z - deltaUV1.y * edge2.z );
	face->tangent   = qm_math_vector3f_normalize( face->tangent );

	face->bitangent.x = f * ( -deltaUV2.x * edge1.x + deltaUV1.x * edge2.x );
	face->bitangent.y = f * ( -deltaUV2.x * edge1.y + deltaUV1.x * edge2.y );
	face->bitangent.z = f * ( -deltaUV2.x * edge1.z + deltaUV1.x * edge2.z );
	face->bitangent   = qm_math_vector3f_normalize( face->bitangent );

#else// my original incorrect approach...

	assert( face->numVertices >= 3 );
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		unsigned int j = ( i + 1 ) % face->numVertices;// next vertex index (wraps around)

		ApeBrushFaceVertex *current = face->edgeLoop[ i ];
		ApeBrushFaceVertex *next    = face->edgeLoop[ j ];
		ApeBrushFaceVertex *prev    = face->edgeLoop[ ( i == 0 ) ? face->numVertices - 1 : ( i - 1 ) ];

		QmMathVector3f dpos1 = qm_math_vector3f_sub( *next->position, *current->position );
		QmMathVector3f dpos2 = qm_math_vector3f_sub( *prev->position, *current->position );

		QmMathVector2f duv1 = PlSubtractVector2( &next->textureCoords, &current->textureCoords );
		QmMathVector2f duv2 = PlSubtractVector2( &prev->textureCoords, &current->textureCoords );

		float r = 1.0f / ( duv1.x * duv2.y - duv1.y * duv2.x );

		QmMathVector3f tangent   = qm_math_vector3f_scale_float( qm_math_vector3f_sub( qm_math_vector3f_scale_float( dpos1, duv2.y ), PlScaleVector3F( dpos2, duv1.y ) ), r );
		QmMathVector3f bitangent = qm_math_vector3f_scale_float( qm_math_vector3f_add( qm_math_vector3f_scale_float( dpos1, -duv2.x ), PlScaleVector3F( dpos2, duv1.x ) ), r );

		current->tangent = next->tangent = prev->tangent = tangent;
		current->bitangent = next->bitangent = prev->bitangent = bitangent;
	}

#endif
}

static void compute_brush_face_texture_coordinates( ApeBrushFace *face, bool computeLocal )
{
	const ApeMaterial *material = face->material;
	assert( material != nullptr );

	unsigned int width  = ape_material_get_width( material );
	unsigned int height = ape_material_get_height( material );

	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	QmMathVector3f verticies[ APE_BRUSH_MAX_FACE_VERTICES ] = {};
	if ( computeLocal )
	{
		QmMathVector3f origin = {};
		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			origin = qm_math_vector3f_add( origin, brush->vertices[ face->vertices[ face->edgeLoopOrder[ i ] ].posIndex ] );
		}

		origin = qm_math_vector3f_div_float( origin, face->numVertices );
		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			verticies[ i ] = qm_math_vector3f_sub( brush->vertices[ face->vertices[ face->edgeLoopOrder[ i ] ].posIndex ], origin );
		}
	}
	else
	{
		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			verticies[ i ] = brush->vertices[ face->vertices[ face->edgeLoopOrder[ i ] ].posIndex ];
		}
	}

	QmMathVector3f up = qm_math_vector3f( 0.0f, 1.0f, 0.0f );
	if ( fabsf( qm_math_vector3f_dot_product( face->normal, up ) ) > 0.99f )
	{
		up = qm_math_vector3f( 1.0f, 0.0f, 0.0f );
	}

	QmMathVector3f u = qm_math_vector3f_normalize( qm_math_vector3f_cross_product( face->normal, up ) );
	QmMathVector3f v = qm_math_vector3f_cross_product( face->normal, u );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		QmMathVector2f coord;
		coord.x = qm_math_vector3f_dot_product( verticies[ i ], u );
		coord.y = qm_math_vector3f_dot_product( verticies[ i ], v );

		// apply rotation
		float ang  = QM_MATH_DEG2RAD( face->materialAngle.x );
		float cos  = cosf( ang );
		float sin  = sinf( ang );
		float rotX = coord.x * cos - coord.y * sin;
		float rotY = coord.x * sin + coord.y * cos;
		coord.x    = rotX;
		coord.y    = rotY;

		face->vertices[ face->edgeLoopOrder[ i ] ].textureCoords.x = ( -coord.x - face->materialOffset.x ) / ( width * face->materialScale.x );
		face->vertices[ face->edgeLoopOrder[ i ] ].textureCoords.y = ( coord.y - face->materialOffset.y ) / ( height * face->materialScale.y );
	}

	compute_brush_face_tangents( face );
}

void ape_brush_face_setup( ApeBrushFace *self )
{
	self->lightmapIndex        = APE_BRUSH_FACE_LIGHTMAP_INVALID;
	self->lightmapLuxelDensity = APE_BRUSH_FACE_LIGHTMAP_DEFAULT_LUXELS;
}

void ape_brush_face_fit_material( ApeBrushFace *self )
{
	for ( unsigned int i = 0; i < self->numVertices; ++i )
	{
		QmMathVector3f up = qm_math_vector3f( 0.0f, 1.0f, 0.0f );
		if ( fabsf( qm_math_vector3f_dot_product( self->normal, up ) ) > 0.99f )
		{
			up = qm_math_vector3f( 1.0f, 0.0f, 0.0f );
		}

		QmMathVector3f u = qm_math_vector3f_normalize( qm_math_vector3f_cross_product( self->normal, up ) );
		QmMathVector3f v = qm_math_vector3f_cross_product( self->normal, u );

		//QmMathVector2f coord;
		//coord.x = qm_math_vector3f_dot_product( *self->edgeLoop[ i ]->position, u );
		//coord.y = qm_math_vector3f_dot_product( *self->edgeLoop[ i ]->position, v );
	}

	const ApeMaterial *material = self->material;
	assert( material != nullptr );

	unsigned int width  = ape_material_get_width( material );
	unsigned int height = ape_material_get_height( material );

	self->materialOffset.x = self->bounds.maxs.x - self->bounds.absOrigin.x / self->materialScale.x;
	self->materialOffset.y = self->bounds.maxs.y - self->bounds.absOrigin.y / self->materialScale.y;

	compute_brush_face_texture_coordinates( self, true );

	// need to notify the parent to update
	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( self->parent ) );
	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}
}

void ape_brush_face_apply_material( ApeBrushFace *self, ApeMaterial *material )
{
	if ( self->material != nullptr )
	{
		ape_material_release( self->material );
	}

	self->material = material;
	//TODO: reference should be added here - for now it's done by caller :(

	// recompute face texture coordinates, as this is relative to the material size
	compute_brush_face_texture_coordinates( self, false );

	// need to notify the parent to update
	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( self->parent ) );
	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}
}

void ape_brush_face_apply_material_coordinates( ApeBrushFace *self, const QmMathVector2f *scale, const QmMathVector2f *offset, const QmMathVector3f *rotation, bool computeLocal )
{
	self->materialScale = *scale;
	self->materialAngle = *rotation;

	//TODO: well this is a cockup, offset is a vec3!? why did I do that... :(
	self->materialOffset.x = offset->x;
	self->materialOffset.y = offset->y;

	// recompute face texture coordinates, as this is relative to the material size
	compute_brush_face_texture_coordinates( self, computeLocal );

	// need to notify the parent to update
	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( self->parent ) );
	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}
}

bool ape_brush_face_is_portal( const ApeBrushFace *self )
{
	if ( self->flags & APE_BRUSH_FACE_FLAG_PORTAL )
	{
		return true;
	}

	return self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags_( self->material ) & APE_MATERIAL_FLAG_MIRROR;
}

bool ape_brush_face_is_mirror( const ApeBrushFace *self )
{
	return self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags_( self->material ) & APE_MATERIAL_FLAG_MIRROR;
}

ApeBrushFace *ape_brush_face_get_portal_destination( ApeBrushFace *self )
{
	if ( self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags_( self->material ) & APE_MATERIAL_FLAG_MIRROR )
	{
		return self;
	}

	if ( self->flags & APE_BRUSH_FACE_FLAG_PORTAL )
	{
		ApeWorld *world = ( ApeWorld * ) ape_world_node_get_root( APE_WORLD_NODE( self->parent ) );
		if ( world == nullptr )
		{
			return nullptr;
		}

		if ( *self->destinationTag == '\0' )
		{
			return nullptr;
		}

		return ape_world_get_tagged_surface( world, self->destinationTag );
	}

	return nullptr;
}

ApeRoom *ape_brush_face_get_room( const ApeBrushFace *self )
{
	ApeBrush *brush = self->parent;
	assert( brush != nullptr );

	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( brush ) );
	assert( room != nullptr );

	return room;
}

bool ape_brush_face_set_tag( ApeBrushFace *self, const char *tag )
{
	// tags are funky, they have a limit of 64 characters,
	// and we need to pass them into a lookup, urgh
	// (who designed this shit, oh right, me...)

	size_t tagLength = strlen( tag );
	if ( tagLength + 1 >= sizeof( self->tag ) )
	{
		ape_console_warning_( "Failed to set tag (%s) for face, tag is too long (%u >= %u)!\n", tag, tagLength, sizeof( self->tag ) );
		return false;
	}

	if ( strcmp( self->tag, tag ) == 0 )
	{
		// tag already set, so nothing to do
		return true;
	}

	ApeRoom *room = ape_brush_face_get_room( self );
	if ( room == nullptr )
	{
		ape_console_warning_( "Failed to set tag (%s) for face, as face isn't attached to a room!\n", tag );
		return false;
	}

	if ( *self->tag != '\0' )
	{
		ape_room_remove_tagged_surface( room, self );
	}

	snprintf( self->tag, sizeof( self->tag ), "%s", tag );
	if ( *self->tag != '\0' )
	{
		ape_room_add_tagged_surface( room, self );
	}

	return true;
}

void ape_brush_face_compute_bounds( ApeBrushFace *face )
{
	assert( face->numVertices > 0 );

	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	face->bounds.mins = qm_math_vector3f( brush->vertices[ face->vertices[ 0 ].posIndex ].x,
	                                      brush->vertices[ face->vertices[ 0 ].posIndex ].y,
	                                      brush->vertices[ face->vertices[ 0 ].posIndex ].z );
	face->bounds.maxs = qm_math_vector3f( brush->vertices[ face->vertices[ 0 ].posIndex ].x,
	                                      brush->vertices[ face->vertices[ 0 ].posIndex ].y,
	                                      brush->vertices[ face->vertices[ 0 ].posIndex ].z );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		for ( unsigned int j = 0; j < 3; ++j )
		{
			if ( PL_VECTOR3_I( brush->vertices[ face->vertices[ i ].posIndex ], j ) > PL_VECTOR3_I( face->bounds.maxs, j ) )
			{
				PL_VECTOR3_I( face->bounds.maxs, j ) = PL_VECTOR3_I( brush->vertices[ face->vertices[ i ].posIndex ], j );
			}
			if ( PL_VECTOR3_I( brush->vertices[ face->vertices[ i ].posIndex ], j ) < PL_VECTOR3_I( face->bounds.mins, j ) )
			{
				PL_VECTOR3_I( face->bounds.mins, j ) = PL_VECTOR3_I( brush->vertices[ face->vertices[ i ].posIndex ], j );
			}
		}
	}

	face->bounds.absOrigin = PlGetAabbAbsOrigin( &face->bounds, face->bounds.origin );
}

void ape_brush_compute_bounds( ApeBrush *self )
{
	assert( self->numVertices > 0 );

	self->base.localBounds.mins = qm_math_vector3f( self->vertices[ 0 ].x, self->vertices[ 0 ].y, self->vertices[ 0 ].z );
	self->base.localBounds.maxs = qm_math_vector3f( self->vertices[ 0 ].x, self->vertices[ 0 ].y, self->vertices[ 0 ].z );
	for ( unsigned int i = 0; i < self->numVertices; ++i )
	{
		for ( unsigned int j = 0; j < 3; ++j )
		{
			if ( PL_VECTOR3_I( self->vertices[ i ], j ) > PL_VECTOR3_I( self->base.localBounds.maxs, j ) )
			{
				PL_VECTOR3_I( self->base.localBounds.maxs, j ) = PL_VECTOR3_I( self->vertices[ i ], j );
			}
			if ( PL_VECTOR3_I( self->vertices[ i ], j ) < PL_VECTOR3_I( self->base.localBounds.mins, j ) )
			{
				PL_VECTOR3_I( self->base.localBounds.mins, j ) = PL_VECTOR3_I( self->vertices[ i ], j );
			}
		}
	}

	QmMathVector3f localPos = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );

	self->base.localBounds.absOrigin = PlGetAabbAbsOrigin( &self->base.localBounds, localPos );

	// sigh...
	ape_world_node_compute_bounds_( APE_WORLD_NODE( self ) );
}

void ape_brush_compute_face_bounds( ApeBrush *self )
{
	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		ape_brush_face_compute_bounds( &self->faces[ i ] );
	}
}

void ape_brush_compute_face_normals( ApeBrush *self )
{
	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		ape_brush_face_compute_normal( &self->faces[ i ] );
	}
}

void ape_brush_flip_face_( ApeBrushFace *face )
{
	unsigned int start = 0;
	unsigned int end   = face->numVertices - 1;
	while ( start < end )
	{
		unsigned int temp            = face->edgeLoopOrder[ start ];
		face->edgeLoopOrder[ start ] = face->edgeLoopOrder[ end ];
		face->edgeLoopOrder[ end ]   = temp;

		start++;
		end--;
	}

	ape_brush_face_compute_normal( face );

	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( face->parent ) );
	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}
}

static void build_block_brush( ApeBrush *self, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f dir, float scale, float signedArea )
{
	self->numVertices = numVertices * 2;
	self->vertices    = QM_OS_MEMORY_NEW_( QmMathVector3f, self->numVertices );
	memcpy( self->vertices, vertices, numVertices * sizeof( QmMathVector3f ) );

	self->numFaces = numVertices + 2;// plus two for top and bottom
	self->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, self->numFaces );
	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		ape_brush_face_setup( &self->faces[ i ] );
	}

	// set up the top and bottom faces first
	self->faces[ 0 ].numVertices = self->faces[ 1 ].numVertices = numVertices;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		// sort out the top and bottom
		self->faces[ 0 ].vertices[ i ].posIndex = i;
		self->faces[ 1 ].vertices[ i ].posIndex = i + numVertices;
		if ( signedArea < 0.0f )
		{
			self->faces[ 0 ].edgeLoopOrder[ numVertices - 1 - i ] = i;
			self->faces[ 1 ].edgeLoopOrder[ i ]                   = i;
		}
		else
		{
			self->faces[ 0 ].edgeLoopOrder[ i ]                   = i;
			self->faces[ 1 ].edgeLoopOrder[ numVertices - 1 - i ] = i;
		}

		// extrude the vertices for the top
		self->vertices[ i + numVertices ] = qm_math_vector3f_add( self->vertices[ i ], qm_math_vector3f_scale_float( dir, scale ) );

		// set up the faces for the edges

		unsigned int next = ( i + 1 ) % numVertices;

		self->faces[ i + 2 ].numVertices            = 4;
		self->faces[ i + 2 ].vertices[ 0 ].posIndex = i;
		self->faces[ i + 2 ].vertices[ 1 ].posIndex = next;
		self->faces[ i + 2 ].vertices[ 2 ].posIndex = next + numVertices;
		self->faces[ i + 2 ].vertices[ 3 ].posIndex = i + numVertices;

		for ( unsigned int j = 0; j < 4; ++j )
		{
			self->faces[ i + 2 ].edgeLoopOrder[ j ] = signedArea < 0.0f ? j : 3 - j;
		}
	}
}

static void build_plane_brush( ApeBrush *self, const QmMathVector3f *vertices, unsigned int numVertices, float signedArea )
{
	self->numVertices = numVertices;
	self->vertices    = QM_OS_MEMORY_NEW_( QmMathVector3f, self->numVertices );
	memcpy( self->vertices, vertices, numVertices * sizeof( QmMathVector3f ) );

	self->numFaces = 1;
	self->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, self->numFaces );
	ape_brush_face_setup( &self->faces[ 0 ] );
	self->faces[ 0 ].numVertices = self->numVertices;
	for ( unsigned int i = 0; i < self->numVertices; ++i )
	{
		self->faces[ 0 ].vertices[ i ].posIndex = i;
		if ( signedArea > 0.0f )
		{
			self->faces[ 0 ].edgeLoopOrder[ numVertices - 1 - i ] = i;
		}
		else
		{
			self->faces[ 0 ].edgeLoopOrder[ i ] = i;
		}
	}
}

bool ape_brush_build_from_polygon_( ApeBrush *self, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f dir, float scale, float signedArea, ApeMaterial *material, ApeEditorBrushType type )
{
	// extrude and build the brush geometry from the given polygon shape
	if ( numVertices < 3 )
	{
		ape_console_warning_( "Invalid number of vertices for building brush from polygon (%u < 3)!\n", numVertices );
		return false;
	}

	switch ( type )
	{
		default:
			ape_console_warning_( "Unsupported brush type (%u)!\n", type );
			return false;
		case APE_EDITOR_BRUSH_TYPE_BLOCK:
			build_block_brush( self, vertices, numVertices, dir, scale, signedArea );
			break;
		case APE_EDITOR_BRUSH_TYPE_PLANE:
			build_plane_brush( self, vertices, numVertices, signedArea );
			break;
	}

	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		self->faces[ i ].parent = self;
		self->faces[ i ].colour = QM_MATH_COLOUR4F( 1.0f, 1.0f, 1.0f, 1.0f );

		self->faces[ i ].material      = material;
		self->faces[ i ].materialScale = ape_editor_get_default_material_scale();

		ape_brush_face_compute_normal( &self->faces[ i ] );
		ape_brush_face_compute_bounds( &self->faces[ i ] );
		compute_brush_face_texture_coordinates( &self->faces[ i ], false );
	}

	ape_brush_compute_bounds( self );

	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( self ) );
	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}

	return true;
}

void ape_brush_mark_parent_dirty( ApeBrush *self )
{
	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( self ) );
	if ( parent == nullptr )
	{
		return;
	}

	ape_world_node_mark_dirty_( parent );
}

void ape_brush_merge_brushes( ApeBrush *self, ApeBrush **brushes, const unsigned int numBrushes )
{
	assert( numBrushes > 0 && brushes != nullptr );

	for ( unsigned int i = 0; i < numBrushes; ++i )
	{
		ApeBrush *brush = brushes[ i ];
		assert( brush != nullptr && brush != self );

		if ( ape_world_node_is_descendant_of_node( APE_WORLD_NODE( self ), APE_WORLD_NODE( brush ) ) )
		{
			ape_console_warning_( "Cannot merge brush, as it is a parent of the brush it's being merged into!\n" );
			brushes[ i ] = nullptr;
			continue;
		}

		// add the new vertices onto the end of the list
		QmMathVector3f *vertices = qm_os_memory_realloc( self->vertices, sizeof( QmMathVector3f ) * ( self->numVertices + brush->numVertices ) );
		if ( vertices == nullptr )
		{
			ape_console_warning_( "Failed to merge brushes: %s\n", PlGetError() );
			brushes[ i ] = nullptr;
			return;
		}

		self->vertices = vertices;
		for ( unsigned int j = 0; j < brush->numVertices; ++j )
		{
			self->vertices[ self->numVertices + j ] = brush->vertices[ j ];
		}

		self->numVertices += brush->numVertices;

		// and now for faces
		ApeBrushFace *faces = qm_os_memory_realloc( self->faces, sizeof( ApeBrushFace ) * ( self->numFaces + brush->numFaces ) );
		if ( faces == nullptr )
		{
			ape_console_warning_( "Failed to merge brushes: %s\n", PlGetError() );
			brushes[ i ] = nullptr;
			continue;
		}

		self->faces = faces;
		for ( unsigned int j = 0; j < brush->numFaces; ++j )
		{
			ApeBrushFace *src = &brush->faces[ j ];
			assert( src != nullptr );

			ApeBrushFace *dst = &self->faces[ self->numFaces + j ];
			assert( dst != nullptr );

			*dst = *src;

			dst->destination = nullptr;

			// we're taking ownership of the ptr now
			// if it's valid, anyway...
			if ( src->ptr != nullptr )
			{
				qm_os_shared_ptr_set( dst->ptr, dst );
				src->ptr = nullptr;
			}

			dst->parent = self;

			for ( unsigned int k = 0; k < src->numVertices; ++k )
			{
				//TODO: do we do material references per face!?

				dst->vertices[ k ].posIndex = src->vertices[ k ].posIndex + self->numVertices - brush->numVertices;
				dst->edgeLoopOrder[ k ]     = src->edgeLoopOrder[ k ];
			}
		}

		self->numFaces += brush->numFaces;
	}

	ApeWorldNode *parent = APE_WORLD_NODE( ape_world_node_get_room( APE_WORLD_NODE( self ) ) );
	if ( parent == nullptr )
	{
		parent = ape_world_node_get_root( APE_WORLD_NODE( self ) );
	}

	assert( parent != nullptr );

	// destroy all the original brushes at the end
	// here, otherwise we might lose children
	for ( unsigned int i = 0; i < numBrushes; ++i )
	{
		ApeBrush *brush = brushes[ i ];

		// any we weren't able to merge are just null now, so skip those
		if ( brush == nullptr )
		{
			continue;
		}

		ApeWorldNode *node = APE_WORLD_NODE( brush );
		ApeWorldNode *child;
		COM_ITERATE_LINKED_LIST( child, node->children, i )
		{
			// move all children to the parent of the brush
			ape_world_node_attach( child, parent );
		}

		ape_world_node_destroy( node );
	}

	ape_brush_compute_bounds( self );
	ape_brush_mark_parent_dirty( self );
}

void ape_brush_smooth_faces( const QmOsLinkedList *faces )
{
	ApeBrushFace *face;

	// clear all the normals
	QM_OS_LINKED_LIST_ITERATE( face, faces, i )
	{
		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			face->vertices[ j ].normal = ( QmMathVector3f ) {};
		}

		// and mark the parent dirty so we regen the meshes
		ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( face->parent ) );
		assert( parent != nullptr );
		ape_world_node_mark_dirty_( parent );
	}

	QM_OS_LINKED_LIST_ITERATE( face, faces, i )
	{
		ApeBrush *brush = face->parent;
		assert( brush != nullptr );

		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			unsigned int  numAdjacentFaces = 0;
			ApeBrushFace *adjacentFaces[ APE_BRUSH_MAX_FACE_VERTICES ];
			ApeBrushFace *adjacentFace;
			QM_OS_LINKED_LIST_ITERATE( adjacentFace, faces, k )
			{
				ApeBrush *adjacentBrush = adjacentFace->parent;
				assert( adjacentBrush != nullptr );

				for ( unsigned int l = 0; l < adjacentFace->numVertices; ++l )
				{
					if ( com_math_vector_check_epsilon( &brush->vertices[ face->vertices[ j ].posIndex ],
					                                    &adjacentBrush->vertices[ adjacentFace->vertices[ l ].posIndex ] ) )
					{
						adjacentFaces[ numAdjacentFaces++ ] = adjacentFace;
						break;
					}
				}

				if ( numAdjacentFaces >= APE_BRUSH_MAX_FACE_VERTICES )
				{
					ape_console_warning_( "Too many adjacent faces to smooth face!\n" );
					break;
				}
			}

			QmMathVector3f normal = {};
			for ( unsigned int k = 0; k < numAdjacentFaces; ++k )
			{
				const ApeBrushFace *adjFace  = adjacentFaces[ k ];
				const ApeBrush     *adjBrush = adjFace->parent;
				assert( adjBrush != nullptr );

				for ( unsigned int l = 0; l < adjFace->numVertices - 2; ++l )
				{
					const QmMathVector3f *a = &adjBrush->vertices[ adjFace->vertices[ adjFace->edgeLoopOrder[ l ] ].posIndex ];
					const QmMathVector3f *b = &adjBrush->vertices[ adjFace->vertices[ adjFace->edgeLoopOrder[ l + 1 ] ].posIndex ];
					const QmMathVector3f *c = &adjBrush->vertices[ adjFace->vertices[ adjFace->edgeLoopOrder[ ( l + 2 ) % adjFace->numVertices ] ].posIndex ];

					QmMathVector3f n = PlgGenerateVertexNormal( *a, *b, *c );

					normal = qm_math_vector3f_add( normal, n );
				}
			}

			face->vertices[ j ].normal = qm_math_vector3f_normalize( normal );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////

static AcmBranch *serialize_brush( void *self, AcmBranch *root )
{
	ApeBrush *brush = self;
	acm_push_ui32( root, "type", brush->type );
	acm_push_array_f32( root, "vertices", ( float * ) brush->vertices, brush->numVertices * 3 );

	AcmBranch *facesBranch = acm_push_array_object( root, "faces" );
	for ( unsigned int i = 0; i < brush->numFaces; ++i )
	{
		const ApeBrushFace *face       = &brush->faces[ i ];
		AcmBranch          *faceBranch = acm_push_object( facesBranch, "face" );

		acm_push_string( faceBranch, "id", face->tag, true );
		acm_push_string( faceBranch, "destination", face->destinationTag, true );

		assert( face->material != nullptr );
		acm_push_string( faceBranch, "material", ape_material_get_path( face->material ), false );
		com_acm_push_vector2( faceBranch, "materialScale", &face->materialScale, true );
		com_acm_push_vector3( faceBranch, "materialOffset", &face->materialOffset, true );
		com_acm_push_vector3( faceBranch, "materialAngle", &face->materialAngle, true );

		com_acm_push_vector3( faceBranch, "normal", &face->normal, true );
		acm_push_array_f32( faceBranch, "colour", ( float * ) &face->colour, 4 );
		acm_push_array_f32( faceBranch, "bounds", ( float * ) &face->bounds, 12 );

		acm_push_ui32( faceBranch, "flags", face->flags );

		acm_push_ui8( faceBranch, "lightmapIndex", face->lightmapIndex );

		AcmBranch *edgeBranch     = acm_push_array_i16( faceBranch, "edgeLoop", nullptr, 0 );
		AcmBranch *verticesBranch = acm_push_array_object( faceBranch, "vertices" );
		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			const ApeBrushFaceVertex *vertex       = &face->vertices[ j ];
			AcmBranch                *vertexBranch = acm_push_object( verticesBranch, "vertex" );
			acm_push_i16( vertexBranch, "position", ( int16_t ) vertex->posIndex );
			com_acm_push_vector2( vertexBranch, "uv", &vertex->textureCoords, true );
			com_acm_push_vector2( vertexBranch, "lightmap", &vertex->lightmapCoords, true );
			com_acm_push_vector3( vertexBranch, "normal", &vertex->normal, true );
			acm_push_array_f32( vertexBranch, "colour", ( float * ) &vertex->colour, 4 );

			acm_push_i16( edgeBranch, "vertex", face->edgeLoopOrder[ j ] );
		}
	}

	return root;
}

static ApeWorldNode *deserialize_brush( ApeWorldNode *self, ApeWorldNode *parent, AcmBranch *root )
{
	ApeBrush *brush = ( ApeBrush * ) self;

	brush->type = ACM_GET_INT( brush->type, root, "type", APE_WORLD_BRUSH_TYPE_SOLID );

	AcmBranch *branch;
	if ( ( branch = acm_get_child_by_name( root, "vertices" ) ) != nullptr )
	{
		brush->numVertices = acm_get_num_of_children( branch ) / 3;
		brush->vertices    = QM_OS_MEMORY_NEW_( QmMathVector3f, brush->numVertices );
		acm_branch_get_float32_array( branch, ( float * ) brush->vertices, brush->numVertices * 3 );
	}
	else
	{
		ape_console_warning_( "No vertices specified for brush!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( brush ) );
		return nullptr;
	}

	if ( ( branch = acm_get_child_by_name( root, "faces" ) ) != nullptr )
	{
		brush->numFaces = acm_get_num_of_children( branch );
		brush->faces    = QM_OS_MEMORY_NEW_( ApeBrushFace, brush->numFaces );

		branch = acm_get_first_child( branch );
		for ( unsigned int i = 0; i < brush->numFaces; ++i, branch = acm_get_next_child( branch ) )
		{
			ape_brush_face_setup( &brush->faces[ i ] );

			brush->faces[ i ].parent = brush;

			AcmBranch *vertexBranch = acm_get_child_by_name( branch, "vertices" );
			if ( vertexBranch != nullptr )
			{
				brush->faces[ i ].numVertices = acm_get_num_of_children( vertexBranch );

				vertexBranch = acm_get_first_child( vertexBranch );
				for ( unsigned int j = 0; j < brush->faces[ i ].numVertices; ++j, vertexBranch = acm_get_next_child( vertexBranch ) )
				{
					brush->faces[ i ].vertices[ j ].posIndex = ACM_GET_INT( brush->faces[ i ].vertices[ j ].posIndex, vertexBranch, "position", 0 );
					assert( brush->faces[ i ].vertices[ j ].posIndex <= brush->numVertices );

					brush->faces[ i ].vertices[ j ].textureCoords  = com_acm_get_vector2( vertexBranch, "uv", &QM_MATH_VECTOR2F_ZERO );
					brush->faces[ i ].vertices[ j ].lightmapCoords = com_acm_get_vector2( vertexBranch, "lightmap", &QM_MATH_VECTOR2F_ZERO );
					brush->faces[ i ].vertices[ j ].normal         = com_acm_get_vector3( vertexBranch, "normal", &( QmMathVector3f ) {} );
					brush->faces[ i ].vertices[ j ].colour         = com_acm_get_colour_f32( vertexBranch, "colour", &( QmMathColour4f ) { .a = 1.0f } );
				}

				int16_t edgeLoop[ APE_BRUSH_MAX_FACE_VERTICES ];
				acm_get_array_i16( branch, "edgeLoop", edgeLoop, brush->faces[ i ].numVertices );
				for ( unsigned int j = 0; j < brush->faces[ i ].numVertices; ++j )
				{
					brush->faces[ i ].edgeLoopOrder[ j ] = edgeLoop[ j ];
				}
			}
			else
			{
				ape_console_warning_( "No vertices specified for brush!\n" );
				ape_world_node_destroy( APE_WORLD_NODE( brush ) );
				return nullptr;
			}

			snprintf( brush->faces[ i ].tag, sizeof( brush->faces[ i ].tag ), "%s", acm_get_string( branch, "id", "" ) );
			if ( *brush->faces[ i ].tag != '\0' )
			{
				ApeRoom *room = ape_brush_face_get_room( &brush->faces[ i ] );
				if ( room != nullptr )
				{
					ape_room_add_tagged_surface( room, &brush->faces[ i ] );
				}
			}

			// material
			const char *str;
			if ( ( str = acm_get_string( branch, "material", nullptr ) ) != nullptr )
			{
				brush->faces[ i ].material = ape_material_cache( str, APE_CACHE_GROUP_WORLD, true );
			}
			else
			{
				ape_console_warning_( "No material specified for a brush face, using default!\n" );
				brush->faces[ i ].material = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR );
			}
			brush->faces[ i ].materialScale  = com_acm_get_vector2( branch, "materialScale", &QM_MATH_VECTOR2F_ZERO );
			brush->faces[ i ].materialOffset = com_acm_get_vector3( branch, "materialOffset", &QM_MATH_VECTOR3F_ZERO );
			brush->faces[ i ].materialAngle  = com_acm_get_vector3( branch, "materialAngle", &QM_MATH_VECTOR3F_ZERO );

			brush->faces[ i ].flags = ACM_GET_UINT( brush->faces[ i ].flags, branch, "flags", 0 );

			brush->faces[ i ].lightmapIndex        = ACM_GET_UINT( brush->faces[ i ].lightmapIndex, branch, "lightmapIndex", APE_BRUSH_FACE_LIGHTMAP_INVALID );
			brush->faces[ i ].lightmapLuxelDensity = ACM_GET_UINT( brush->faces[ i ].lightmapLuxelDensity, branch, "lightmapLuxelDensity", APE_BRUSH_FACE_LIGHTMAP_DEFAULT_LUXELS );

			brush->faces[ i ].normal = com_acm_get_vector3( branch, "normal", &QM_MATH_VECTOR3F_ZERO );
			brush->faces[ i ].colour = com_acm_get_colour_f32( branch, "colour", &( QmMathColour4f ) { .a = 1.0f } );
			acm_get_array_f32( branch, "bounds", ( float * ) &brush->faces[ i ].bounds, 12 );

			compute_brush_face_tangents( &brush->faces[ i ] );

			ape_brush_compute_bounds( brush );
		}
	}
	else
	{
		ape_console_warning_( "No faces specified for brush!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( brush ) );
		return nullptr;
	}

	return APE_WORLD_NODE( brush );
}

static void on_change_room( void *self, ApeRoom *currentRoom, ApeRoom *newRoom )
{
	for ( unsigned int i = 0; i < ( ( ApeBrush * ) self )->numFaces; ++i )
	{
		ApeBrushFace *face = &( ( ApeBrush * ) self )->faces[ i ];
		if ( *face->tag == '\0' )
		{
			continue;
		}

		ape_room_remove_tagged_surface( currentRoom, face );
		ape_room_add_tagged_surface( newRoom, face );
	}
}

static ApePropertyEnum brushTypeEnums[] = {
        {"Solid", 0},
        {"Air",   1},
};

static const ApeProperty properties[] = {
        APE_PROPERTY_ENUM( "Type", "Type of brush, which can either be solid or air.", ApeBrush, type, brushTypeEnums ),
};

const ApeWorldNodeClass ape_brushClass = {
        .identifier = "brush",
        .magic      = QM_OS_MAGIC_TO_NUM( 'B', 'R', 'S', 'H' ),

        .create      = create_brush,
        .destroy     = destroy_brush,
        .serialize   = serialize_brush,
        .deserialize = deserialize_brush,

        .onChangeRoom = on_change_room,

        .clone = clone_brush,

        .properties    = properties,
        .numProperties = QM_OS_ARRAY_ELEMENTS( properties ),

        .flags = APE_WORLD_NODE_CLASS_FLAG_NO_EDITOR,
};
