// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

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

ApeBrush *ape_brush_create( ApeWorldNode *parent, const char *name, const PLVector3 *position, const PLVector3 *angles )
{
	ApeBrush *brush = PL_NEW( ApeBrush );
	if ( ape_world_node_setup_( &brush->base, parent, APE_WORLD_NODE_TYPE_BRUSH, name, position, angles ) == nullptr )
	{
		PL_DELETEN( brush );
	}

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

	PL_DELETE( self->vertices );

	for ( unsigned int i = 0; i < self->numFaces; ++i )
	{
		if ( self->faces[ i ].ptr == nullptr )
		{
			continue;
		}

		com_shared_ptr_release( self->faces[ i ].ptr );
		com_shared_ptr_set( self->faces[ i ].ptr, nullptr );
		self->faces[ i ].ptr = nullptr;
	}
	PL_DELETE( self->faces );

#if !defined( APE_NO_EDITOR )
	PL_DELETE( self->vertexSelectColours );
#endif

	PL_DELETE( self );
}

static ApeWorldNode *clone_brush( ApeWorldNode *src )
{
	ApeBrush *srcBrush = ( ApeBrush * ) src;
	ApeBrush *dstBrush = ape_brush_create( src->parent, src->name, &src->position, &src->angles );
	if ( dstBrush == nullptr )
	{
		ape_warning_( "Failed to create brush for duplication!\n" );
		return nullptr;
	}

	dstBrush->type = srcBrush->type;

	dstBrush->numVertices = srcBrush->numVertices;
	dstBrush->vertices    = PL_NEW_( PLVector3, dstBrush->numVertices );
	for ( unsigned int j = 0; j < dstBrush->numVertices; ++j )
	{
		dstBrush->vertices[ j ] = srcBrush->vertices[ j ];
	}

	dstBrush->numFaces = srcBrush->numFaces;
	dstBrush->faces    = PL_NEW_( ApeBrushFace, dstBrush->numFaces );
	for ( unsigned int j = 0; j < dstBrush->numFaces; ++j )
	{
		//TODO: materials are an annoying pain in the ass because of how we're handling references... this should be fixed...
		const char  *materialPath     = ape_material_get_path( srcBrush->faces[ j ].material );
		ApeMaterial *material         = ape_material_cache( materialPath, APE_CACHE_GROUP_WORLD, true );
		dstBrush->faces[ j ].material = material;

		dstBrush->faces[ j ].materialScale  = srcBrush->faces[ j ].materialScale;
		dstBrush->faces[ j ].materialOffset = srcBrush->faces[ j ].materialOffset;
		dstBrush->faces[ j ].materialAngle  = srcBrush->faces[ j ].materialAngle;

		dstBrush->faces[ j ].normal = srcBrush->faces[ j ].normal;

		dstBrush->faces[ j ].flags = srcBrush->faces[ j ].flags;

		dstBrush->faces[ j ].bounds      = srcBrush->faces[ j ].bounds;
		dstBrush->faces[ j ].numVertices = srcBrush->faces[ j ].numVertices;
		for ( unsigned int k = 0; k < dstBrush->faces[ j ].numVertices; ++k )
		{
			dstBrush->faces[ j ].vertices[ k ].position      = &dstBrush->vertices[ srcBrush->faces[ j ].vertices[ k ].position - srcBrush->vertices ];
			dstBrush->faces[ j ].vertices[ k ].textureCoords = srcBrush->faces[ j ].vertices[ k ].textureCoords;
			dstBrush->faces[ j ].vertices[ k ].normal        = srcBrush->faces[ j ].vertices[ k ].normal;

			// and now for the edge loop...
			dstBrush->faces[ j ].edgeLoop[ k ] = &dstBrush->faces[ j ].vertices[ srcBrush->faces[ j ].edgeLoop[ k ] - srcBrush->faces[ j ].vertices ];
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
	face->normal = PL_VECTOR3( 0.0f, 0.0f, 0.0f );

	assert( face->numVertices >= 3 );
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		unsigned int j = ( i + 1 ) % face->numVertices;// next vertex index (wraps around)

		const PLVector3 *current = face->edgeLoop[ i ]->position;
		const PLVector3 *next    = face->edgeLoop[ j ]->position;
		const PLVector3 *prev    = face->edgeLoop[ ( i == 0 ) ? face->numVertices - 1 : ( i - 1 ) ]->position;

		PLVector3 edge1 = PL_VECTOR3( next->x - current->x, next->y - current->y, next->z - current->z );
		PLVector3 edge2 = PL_VECTOR3( prev->x - current->x, prev->y - current->y, prev->z - current->z );

		PLVector3 n  = PlVector3CrossProduct( edge1, edge2 );
		face->normal = PlAddVector3( face->normal, n );
	}

	face->normal = PlNormalizeVector3( face->normal );
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		face->vertices[ i ].normal = face->normal;
	}

	// store the normal here, as the face normal may be modified later
	face->plane.normal = face->normal;
}

static void compute_brush_face_tangents( ApeBrushFace *face )
{
	face->tangent = face->bitangent = ( PLVector3 ) {};

	assert( face->numVertices >= 3 );

#if 1

	ApeBrushFaceVertex *v0 = face->edgeLoop[ 0 ];
	ApeBrushFaceVertex *v1 = face->edgeLoop[ 1 ];
	ApeBrushFaceVertex *v2 = face->edgeLoop[ 2 ];

	PLVector3 edge1 = PlSubtractVector3( *v1->position, *v0->position );
	PLVector3 edge2 = PlSubtractVector3( *v2->position, *v0->position );

	PLVector2 deltaUV1 = PlSubtractVector2( &v1->textureCoords, &v0->textureCoords );
	PLVector2 deltaUV2 = PlSubtractVector2( &v2->textureCoords, &v0->textureCoords );

	float f = 1.0f / ( deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y );

	face->tangent.x = f * ( deltaUV2.y * edge1.x - deltaUV1.y * edge2.x );
	face->tangent.y = f * ( deltaUV2.y * edge1.y - deltaUV1.y * edge2.y );
	face->tangent.z = f * ( deltaUV2.y * edge1.z - deltaUV1.y * edge2.z );
	face->tangent   = PlNormalizeVector3( face->tangent );

	face->bitangent.x = f * ( -deltaUV2.x * edge1.x + deltaUV1.x * edge2.x );
	face->bitangent.y = f * ( -deltaUV2.x * edge1.y + deltaUV1.x * edge2.y );
	face->bitangent.z = f * ( -deltaUV2.x * edge1.z + deltaUV1.x * edge2.z );
	face->bitangent   = PlNormalizeVector3( face->bitangent );

#else// my original incorrect approach...

	assert( face->numVertices >= 3 );
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		unsigned int j = ( i + 1 ) % face->numVertices;// next vertex index (wraps around)

		ApeBrushFaceVertex *current = face->edgeLoop[ i ];
		ApeBrushFaceVertex *next    = face->edgeLoop[ j ];
		ApeBrushFaceVertex *prev    = face->edgeLoop[ ( i == 0 ) ? face->numVertices - 1 : ( i - 1 ) ];

		PLVector3 dpos1 = PlSubtractVector3( *next->position, *current->position );
		PLVector3 dpos2 = PlSubtractVector3( *prev->position, *current->position );

		PLVector2 duv1 = PlSubtractVector2( &next->textureCoords, &current->textureCoords );
		PLVector2 duv2 = PlSubtractVector2( &prev->textureCoords, &current->textureCoords );

		float r = 1.0f / ( duv1.x * duv2.y - duv1.y * duv2.x );

		PLVector3 tangent   = PlScaleVector3F( PlSubtractVector3( PlScaleVector3F( dpos1, duv2.y ), PlScaleVector3F( dpos2, duv1.y ) ), r );
		PLVector3 bitangent = PlScaleVector3F( PlAddVector3( PlScaleVector3F( dpos1, -duv2.x ), PlScaleVector3F( dpos2, duv1.x ) ), r );

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

	PLVector3 verticies[ APE_BRUSH_MAX_FACE_VERTICES ] = {};
	if ( computeLocal )
	{
		PLVector3 origin = {};
		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			origin = PlAddVector3( origin, *face->edgeLoop[ i ]->position );
		}

		origin = PlDivideVector3F( origin, face->numVertices );
		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			verticies[ i ] = PlSubtractVector3( *face->edgeLoop[ i ]->position, origin );
		}
	}
	else
	{
		for ( unsigned int i = 0; i < face->numVertices; ++i )
		{
			verticies[ i ] = *face->edgeLoop[ i ]->position;
		}
	}

	PLVector3 up = PL_VECTOR3( 0.0f, 1.0f, 0.0f );
	if ( fabsf( PlVector3DotProduct( face->normal, up ) ) > 0.99f )
	{
		up = PL_VECTOR3( 1.0f, 0.0f, 0.0f );
	}

	PLVector3 u = PlNormalizeVector3( PlVector3CrossProduct( face->normal, up ) );
	PLVector3 v = PlVector3CrossProduct( face->normal, u );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		PLVector2 coord;
		coord.x = PlVector3DotProduct( verticies[ i ], u );
		coord.y = PlVector3DotProduct( verticies[ i ], v );

		// apply rotation
		float ang  = PL_DEG2RAD( face->materialAngle.x );
		float cos  = cosf( ang );
		float sin  = sinf( ang );
		float rotX = coord.x * cos - coord.y * sin;
		float rotY = coord.x * sin + coord.y * cos;
		coord.x    = rotX;
		coord.y    = rotY;

		face->edgeLoop[ i ]->textureCoords.x = ( -coord.x - face->materialOffset.x ) / ( width * face->materialScale.x );
		face->edgeLoop[ i ]->textureCoords.y = ( coord.y - face->materialOffset.y ) / ( height * face->materialScale.y );
	}
}

void ape_brush_face_fit_material( ApeBrushFace *self )
{
	for ( unsigned int i = 0; i < self->numVertices; ++i )
	{
		PLVector3 up = PL_VECTOR3( 0.0f, 1.0f, 0.0f );
		if ( fabsf( PlVector3DotProduct( self->normal, up ) ) > 0.99f )
		{
			up = PL_VECTOR3( 1.0f, 0.0f, 0.0f );
		}

		PLVector3 u = PlNormalizeVector3( PlVector3CrossProduct( self->normal, up ) );
		PLVector3 v = PlVector3CrossProduct( self->normal, u );

		//PLVector2 coord;
		//coord.x = PlVector3DotProduct( *self->edgeLoop[ i ]->position, u );
		//coord.y = PlVector3DotProduct( *self->edgeLoop[ i ]->position, v );
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

void ape_brush_face_apply_material_coordinates( ApeBrushFace *self, const PLVector2 *scale, const PLVector2 *offset, const PLVector3 *rotation, bool computeLocal )
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

	return self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags( self->material ) & APE_MATERIAL_FLAG_MIRROR;
}

bool ape_brush_face_is_mirror( const ApeBrushFace *self )
{
	return self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags( self->material ) & APE_MATERIAL_FLAG_MIRROR;
}

ApeBrushFace *ape_brush_face_get_portal_destination( ApeBrushFace *self )
{
	if ( self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags( self->material ) & APE_MATERIAL_FLAG_MIRROR )
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
		ape_warning_( "Failed to set tag (%s) for face, tag is too long (%u >= %u)!\n", tag, tagLength, sizeof( self->tag ) );
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
		ape_warning_( "Failed to set tag (%s) for face, as face isn't attached to a room!\n", tag );
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

	face->bounds.mins = PL_VECTOR3( face->vertices[ 0 ].position->x, face->vertices[ 0 ].position->y, face->vertices[ 0 ].position->z );
	face->bounds.maxs = PL_VECTOR3( face->vertices[ 0 ].position->x, face->vertices[ 0 ].position->y, face->vertices[ 0 ].position->z );
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		for ( unsigned int j = 0; j < 3; ++j )
		{
			if ( PL_VECTOR3_I( *face->vertices[ i ].position, j ) > PL_VECTOR3_I( face->bounds.maxs, j ) )
			{
				PL_VECTOR3_I( face->bounds.maxs, j ) = PL_VECTOR3_I( *face->vertices[ i ].position, j );
			}
			if ( PL_VECTOR3_I( *face->vertices[ i ].position, j ) < PL_VECTOR3_I( face->bounds.mins, j ) )
			{
				PL_VECTOR3_I( face->bounds.mins, j ) = PL_VECTOR3_I( *face->vertices[ i ].position, j );
			}
		}
	}

	face->bounds.absOrigin = PlGetAabbAbsOrigin( &face->bounds, face->bounds.origin );
}

void ape_brush_compute_bounds( ApeBrush *self )
{
	assert( self->numVertices > 0 );

	self->base.localBounds.mins = PL_VECTOR3( self->vertices[ 0 ].x, self->vertices[ 0 ].y, self->vertices[ 0 ].z );
	self->base.localBounds.maxs = PL_VECTOR3( self->vertices[ 0 ].x, self->vertices[ 0 ].y, self->vertices[ 0 ].z );
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

	PLVector3 localPos = ape_world_node_get_local_position( APE_WORLD_NODE( self ) );

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
		ApeBrushFaceVertex *temp = face->edgeLoop[ start ];
		face->edgeLoop[ start ]  = face->edgeLoop[ end ];
		face->edgeLoop[ end ]    = temp;

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

static void build_block_brush( ApeBrush *self, const PLVector3 *vertices, unsigned int numVertices, PLVector3 dir, float scale, float signedArea )
{
	self->numVertices = numVertices * 2;
	self->vertices    = PL_NEW_( PLVector3, self->numVertices );
	memcpy( self->vertices, vertices, numVertices * sizeof( PLVector3 ) );

	self->numFaces = numVertices + 2;// plus two for top and bottom
	self->faces    = PL_NEW_( ApeBrushFace, self->numFaces );

	// set up the top and bottom faces first
	self->faces[ 0 ].numVertices = self->faces[ 1 ].numVertices = numVertices;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		// sort out the top and bottom
		self->faces[ 0 ].vertices[ i ].position = &self->vertices[ i ];
		self->faces[ 1 ].vertices[ i ].position = &self->vertices[ i + numVertices ];
		if ( signedArea < 0.0f )
		{
			self->faces[ 0 ].edgeLoop[ numVertices - 1 - i ] = &self->faces[ 0 ].vertices[ i ];
			self->faces[ 1 ].edgeLoop[ i ]                   = &self->faces[ 1 ].vertices[ i ];
		}
		else
		{
			self->faces[ 0 ].edgeLoop[ i ]                   = &self->faces[ 0 ].vertices[ i ];
			self->faces[ 1 ].edgeLoop[ numVertices - 1 - i ] = &self->faces[ 1 ].vertices[ i ];
		}

		// extrude the vertices for the top
		self->vertices[ i + numVertices ] = PlAddVector3( self->vertices[ i ], PlScaleVector3F( dir, scale ) );

		// set up the faces for the edges

		unsigned int next = ( i + 1 ) % numVertices;

		PLVector3 *quad[ 4 ];
		quad[ 0 ] = &self->vertices[ i ];
		quad[ 1 ] = &self->vertices[ next ];
		quad[ 2 ] = &self->vertices[ i + numVertices ];
		quad[ 3 ] = &self->vertices[ next + numVertices ];

		self->faces[ i + 2 ].numVertices            = 4;
		self->faces[ i + 2 ].vertices[ 0 ].position = quad[ 0 ];
		self->faces[ i + 2 ].vertices[ 1 ].position = quad[ 1 ];
		self->faces[ i + 2 ].vertices[ 2 ].position = quad[ 3 ];
		self->faces[ i + 2 ].vertices[ 3 ].position = quad[ 2 ];

		for ( unsigned int j = 0; j < 4; ++j )
		{
			self->faces[ i + 2 ].edgeLoop[ j ] = ( signedArea < 0.0f ) ? &self->faces[ i + 2 ].vertices[ j ] : &self->faces[ i + 2 ].vertices[ 3 - j ];
		}
	}
}

static void build_plane_brush( ApeBrush *self, const PLVector3 *vertices, unsigned int numVertices, float signedArea )
{
	self->numVertices = numVertices;
	self->vertices    = PL_NEW_( PLVector3, self->numVertices );
	memcpy( self->vertices, vertices, numVertices * sizeof( PLVector3 ) );

	self->numFaces               = 1;
	self->faces                  = PL_NEW_( ApeBrushFace, self->numFaces );
	self->faces[ 0 ].numVertices = self->numVertices;
	for ( unsigned int i = 0; i < self->numVertices; ++i )
	{
		self->faces[ 0 ].vertices[ i ].position = &self->vertices[ i ];
		if ( signedArea > 0.0f )
		{
			self->faces[ 0 ].edgeLoop[ numVertices - 1 - i ] = &self->faces[ 0 ].vertices[ i ];
		}
		else
		{
			self->faces[ 0 ].edgeLoop[ i ] = &self->faces[ 0 ].vertices[ i ];
		}
	}
}

bool ape_brush_build_from_polygon_( ApeBrush *self, const PLVector3 *vertices, unsigned int numVertices, PLVector3 dir, float scale, float signedArea, ApeMaterial *material, ApeEditorBrushType type )
{
	// extrude and build the brush geometry from the given polygon shape
	if ( numVertices < 3 )
	{
		ape_warning_( "Invalid number of vertices for building brush from polygon (%u < 3)!\n", numVertices );
		return false;
	}

	switch ( type )
	{
		default:
			ape_warning_( "Unsupported brush type (%u)!\n", type );
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
		self->faces[ i ].colour = PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.0f );

		self->faces[ i ].material      = material;
		self->faces[ i ].materialScale = PL_VECTOR2( 0.5f, 0.5f );

		ape_brush_face_compute_normal( &self->faces[ i ] );
		ape_brush_face_compute_bounds( &self->faces[ i ] );
		compute_brush_face_texture_coordinates( &self->faces[ i ], false );
		compute_brush_face_tangents( &self->faces[ i ] );
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

		AcmBranch *edgeBranch     = acm_push_array_i16( faceBranch, "edgeLoop", nullptr, 0 );
		AcmBranch *verticesBranch = acm_push_array_object( faceBranch, "vertices" );
		for ( unsigned int j = 0; j < face->numVertices; ++j )
		{
			const ApeBrushFaceVertex *vertex       = &face->vertices[ j ];
			AcmBranch                *vertexBranch = acm_push_object( verticesBranch, "vertex" );
			acm_push_i16( vertexBranch, "position", vertex->position - brush->vertices );
			com_acm_push_vector2( vertexBranch, "uv", &vertex->textureCoords, true );
			com_acm_push_vector3( vertexBranch, "normal", &vertex->normal, true );
			acm_push_array_f32( vertexBranch, "colour", ( float * ) &vertex->colour, 4 );

			acm_push_i16( edgeBranch, "vertex", face->edgeLoop[ j ] - face->vertices );
		}
	}

	return root;
}

static ApeWorldNode *deserialize_brush( ApeWorldNode *parent, AcmBranch *root )
{
	ApeBrush *self = ape_brush_create( parent, "temp", &( PLVector3 ) {}, &( PLVector3 ) {} );

	self->type = ACM_GET_INT( self->type, root, "type", APE_WORLD_BRUSH_TYPE_SOLID );

	AcmBranch *branch;
	if ( ( branch = acm_get_child_by_name( root, "vertices" ) ) != nullptr )
	{
		self->numVertices = acm_get_num_of_children( branch ) / 3;
		self->vertices    = PL_NEW_( PLVector3, self->numVertices );
		acm_branch_get_float32_array( branch, ( float * ) self->vertices, self->numVertices * 3 );
	}
	else
	{
		ape_warning_( "No vertices specified for brush!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( self ) );
		return nullptr;
	}

	if ( ( branch = acm_get_child_by_name( root, "faces" ) ) != nullptr )
	{
		self->numFaces = acm_get_num_of_children( branch );
		self->faces    = PL_NEW_( ApeBrushFace, self->numFaces );

		branch = acm_get_first_child( branch );
		for ( unsigned int i = 0; i < self->numFaces; ++i, branch = acm_get_next_child( branch ) )
		{
			self->faces[ i ].parent = self;

			AcmBranch *vertexBranch = acm_get_child_by_name( branch, "vertices" );
			if ( vertexBranch != nullptr )
			{
				self->faces[ i ].numVertices = acm_get_num_of_children( vertexBranch );

				vertexBranch = acm_get_first_child( vertexBranch );
				for ( unsigned int j = 0; j < self->faces[ i ].numVertices; ++j, vertexBranch = acm_get_next_child( vertexBranch ) )
				{
					int16_t vertexIndex = ACM_GET_INT( vertexIndex, vertexBranch, "position", 0 );
					assert( vertexIndex <= self->numVertices );
					self->faces[ i ].vertices[ j ].position      = &self->vertices[ vertexIndex ];
					self->faces[ i ].vertices[ j ].textureCoords = com_acm_get_vector2( vertexBranch, "uv", &( PLVector2 ) {} );
					self->faces[ i ].vertices[ j ].normal        = com_acm_get_vector3( vertexBranch, "normal", &( PLVector3 ) {} );
					self->faces[ i ].vertices[ j ].colour        = com_acm_get_colour_f32( vertexBranch, "colour", &( PLColourF32 ) { .a = 1.0f } );
				}

				int16_t edgeLoop[ APE_BRUSH_MAX_FACE_VERTICES ];
				acm_get_array_i16( branch, "edgeLoop", edgeLoop, self->faces[ i ].numVertices );
				for ( unsigned int j = 0; j < self->faces[ i ].numVertices; ++j )
				{
					self->faces[ i ].edgeLoop[ j ] = &self->faces[ i ].vertices[ edgeLoop[ j ] ];
				}
			}
			else
			{
				ape_warning_( "No vertices specified for brush!\n" );
				ape_world_node_destroy( APE_WORLD_NODE( self ) );
				return nullptr;
			}

			snprintf( self->faces[ i ].tag, sizeof( self->faces[ i ].tag ), "%s", acm_get_string( branch, "id", "" ) );
			if ( *self->faces[ i ].tag != '\0' )
			{
				ApeRoom *room = ape_brush_face_get_room( &self->faces[ i ] );
				if ( room != nullptr )
				{
					ape_room_add_tagged_surface( room, &self->faces[ i ] );
				}
			}

			// material
			const char *str;
			if ( ( str = acm_get_string( branch, "material", nullptr ) ) != nullptr )
			{
				self->faces[ i ].material = ape_material_cache( str, APE_CACHE_GROUP_WORLD, true );
			}
			else
			{
				ape_warning_( "No material specified for a brush face, using default!\n" );
				self->faces[ i ].material = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR );
			}
			self->faces[ i ].materialScale  = com_acm_get_vector2( branch, "materialScale", &( PLVector2 ) {} );
			self->faces[ i ].materialOffset = com_acm_get_vector3( branch, "materialOffset", &( PLVector3 ) {} );
			self->faces[ i ].materialAngle  = com_acm_get_vector3( branch, "materialAngle", &( PLVector3 ) {} );

			self->faces[ i ].flags = ACM_GET_UINT( self->faces[ i ].flags, branch, "flags", 0 );

			self->faces[ i ].normal = com_acm_get_vector3( branch, "normal", &( PLVector3 ) {} );
			self->faces[ i ].colour = com_acm_get_colour_f32( branch, "colour", &( PLColourF32 ) { .a = 1.0f } );
			acm_get_array_f32( branch, "bounds", ( float * ) &self->faces[ i ].bounds, 12 );

			compute_brush_face_tangents( &self->faces[ i ] );
			ape_brush_compute_bounds( self );
		}
	}
	else
	{
		ape_warning_( "No faces specified for brush!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( self ) );
		return nullptr;
	}

	return APE_WORLD_NODE( self );
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

static ApeWorldNodePropertyEnum brushTypeEnums[] = {
        {"Solid", 0},
        {"Air",   1},
};

static const ApeWorldNodeProperty properties[] = {
        APE_WORLD_NODE_PROPERTY_ENUM( "Type", "Type of brush, which can either be solid or air.", ApeBrush, type, brushTypeEnums ),
};

const ApeWorldNodeClass ape_brushClass = {
        .identifier = "brush",
        .magic      = QM_OS_MAGIC_TO_NUM( 'B', 'R', 'S', 'H' ),

        .destroy     = destroy_brush,
        .serialize   = serialize_brush,
        .deserialize = deserialize_brush,

        .onChangeRoom = on_change_room,

        .clone = clone_brush,

        .properties    = properties,
        .numProperties = PL_ARRAY_ELEMENTS( properties ),

        .flags = APE_WORLD_NODE_CLASS_FLAG_NO_EDITOR,
};
