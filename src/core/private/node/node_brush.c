// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "world/world.h"
#include "client/renderer/material/material.h"
#include "client/renderer/renderer.h"

ApeBrush *ape_brush_create( ApeWorldNode *parent, const char *name, const PLVector3 *position, const PLVector3 *angles )
{
	ApeBrush *brush = PL_NEW( ApeBrush );
	if ( ape_world_node_setup_( &brush->base, parent, APE_WORLD_NODE_TYPE_BRUSH, name, position, angles ) == nullptr )
	{
		PL_DELETEN( brush );
	}

	return brush;
}

void ape_brush_destroy_( void *data, ApeWorldNode *parent )
{
	ApeBrush *self = ( ApeBrush * ) data;
	if ( self == nullptr )
	{
		return;
	}

	//HACK: notify the room it's rebuild time!
	ApeRoom *room = ape_world_node_get_room( parent );
	if ( room != nullptr )
	{
		room->isDirty = true;
	}

	PL_DELETE( self->vertices );
	PL_DELETE( self->faces );

	PL_DELETE( self );
}

static uint convert_brush_polygon_to_triangles( const ApeBrushFace *face, uint *indices )
{
	assert( face->numVertices >= 3 );

	uint  numTriangles = 0;
	uint *index        = indices;

#if 0// concave polygon

	/**
	 * Here's an algorithm I think could work - two passes...
	 * 	1. Follow edge loop as we do for convex, but if there is an overlap, skip and mark
	 * 	2. Now continue on from those we skipped in a similar way to the above, with the first skipped being the start, if there is another overlap then repeat for those
	 *
	 * Math isn't my strong point, so it's probably dumb.
	 */

#else// convex polygon

	for ( uint i = 1; i + 1 < face->numVertices; ++i )
	{
		index[ 0 ] = ( face->edgeLoop[ 0 ] - face->vertices );
		index[ 1 ] = ( face->edgeLoop[ i ] - face->vertices );
		index[ 2 ] = ( face->edgeLoop[ i + 1 ] - face->vertices );
		index += 3;

		numTriangles++;
	}

#endif

	return numTriangles;
}

static void compute_brush_face_normal( ApeBrushFace *face )
{
	face->normal = PL_VECTOR3( 0.0f, 0.0f, 0.0f );

	assert( face->numVertices >= 3 );
	for ( uint i = 0; i < face->numVertices; ++i )
	{
		uint j = ( i + 1 ) % face->numVertices;// next vertex index (wraps around)

		const PLVector3 *current = face->edgeLoop[ i ]->position;
		const PLVector3 *next    = face->edgeLoop[ j ]->position;
		const PLVector3 *prev    = face->edgeLoop[ ( i == 0 ) ? face->numVertices - 1 : ( i - 1 ) ]->position;

		PLVector3 edge1 = PL_VECTOR3( next->x - current->x, next->y - current->y, next->z - current->z );
		PLVector3 edge2 = PL_VECTOR3( prev->x - current->x, prev->y - current->y, prev->z - current->z );

		PLVector3 n  = PlVector3CrossProduct( edge1, edge2 );
		face->normal = PlAddVector3( face->normal, n );
	}

	face->normal = PlNormalizeVector3( face->normal );
	for ( uint i = 0; i < face->numVertices; ++i )
	{
		face->vertices[ i ].normal = face->normal;
	}
}

static void compute_brush_face_tangents( ApeBrushFace *face )
{
	assert( face->numVertices >= 3 );
	for ( uint i = 0; i < face->numVertices; ++i )
	{
		uint j = ( i + 1 ) % face->numVertices;// next vertex index (wraps around)

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
}

static void compute_brush_face_texture_coordinates( ApeBrushFace *face )
{
	for ( uint i = 0; i < face->numVertices; ++i )
	{
		PLVector3 up = PL_VECTOR3( 0.0f, 1.0f, 0.0f );
		if ( fabsf( PlVector3DotProduct( face->normal, up ) ) > 0.99f )
		{
			up = PL_VECTOR3( 1.0f, 0.0f, 0.0f );
		}

		PLVector3 u = PlNormalizeVector3( PlVector3CrossProduct( face->normal, up ) );
		PLVector3 v = PlVector3CrossProduct( face->normal, u );

		PLVector2 coord;
		coord.x = PlVector3DotProduct( *face->edgeLoop[ i ]->position, u );
		coord.y = PlVector3DotProduct( *face->edgeLoop[ i ]->position, v );

		ApeMaterial *material = face->material;
		assert( material != nullptr );
		uint width  = ape_material_get_width( material );
		uint height = ape_material_get_height( material );

		face->edgeLoop[ i ]->textureCoords.x = ( -coord.x - face->materialOffset.x ) / ( width * face->materialScale.x );
		face->edgeLoop[ i ]->textureCoords.y = ( coord.y - face->materialOffset.y ) / ( height * face->materialScale.y );
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
	compute_brush_face_texture_coordinates( self );

	// need to notify the room to update
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( self->parent ) );
	assert( room != nullptr );
	room->isDirty = true;
}

bool ape_brush_face_is_portal( const ApeBrushFace *self )
{
	if ( self->destination != nullptr )
	{
		return true;
	}

	return self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags( self->material ) & APE_MATERIAL_FLAG_MIRROR;
}

ApeBrushFace *ape_brush_face_get_portal_destination( ApeBrushFace *self )
{
	if ( self->destination != nullptr )
	{
		return self->destination;
	}

	if ( self->flags & APE_BRUSH_FACE_FLAG_MIRROR || ape_material_get_flags( self->material ) & APE_MATERIAL_FLAG_MIRROR )
	{
		return self;
	}

	return nullptr;
}

ApeRoom *ape_brush_face_get_room( ApeBrushFace *self )
{
	ApeBrush *brush = self->parent;
	assert( brush != nullptr );

	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( brush ) );
	assert( room != nullptr );

	return room;
}

static void compute_brush_face_bounds( ApeBrushFace *face )
{
	assert( face->numVertices > 0 );

	face->bounds.mins = PL_VECTOR3( face->vertices[ 0 ].position->x, face->vertices[ 0 ].position->y, face->vertices[ 0 ].position->z );
	face->bounds.maxs = PL_VECTOR3( face->vertices[ 0 ].position->x, face->vertices[ 0 ].position->y, face->vertices[ 0 ].position->z );
	for ( uint i = 0; i < face->numVertices; ++i )
	{
		for ( uint j = 0; j < 3; ++j )
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

static void compute_brush_bounds( ApeBrush *self )
{
	assert( self->numVertices > 0 );

	self->base.bounds.mins = PL_VECTOR3( self->vertices[ 0 ].x, self->vertices[ 0 ].y, self->vertices[ 0 ].z );
	self->base.bounds.maxs = PL_VECTOR3( self->vertices[ 0 ].x, self->vertices[ 0 ].y, self->vertices[ 0 ].z );
	for ( uint i = 0; i < self->numVertices; ++i )
	{
		for ( uint j = 0; j < 3; ++j )
		{
			if ( PL_VECTOR3_I( self->vertices[ i ], j ) > PL_VECTOR3_I( self->base.bounds.maxs, j ) )
			{
				PL_VECTOR3_I( self->base.bounds.maxs, j ) = PL_VECTOR3_I( self->vertices[ i ], j );
			}
			if ( PL_VECTOR3_I( self->vertices[ i ], j ) < PL_VECTOR3_I( self->base.bounds.mins, j ) )
			{
				PL_VECTOR3_I( self->base.bounds.mins, j ) = PL_VECTOR3_I( self->vertices[ i ], j );
			}
		}
	}

	self->base.bounds.absOrigin = PlGetAabbAbsOrigin( &self->base.bounds, self->base.position );
}

void ape_brush_flip_face_( ApeBrushFace *face )
{
	uint start = 0;
	uint end   = face->numVertices - 1;
	while ( start < end )
	{
		ApeBrushFaceVertex *temp = face->edgeLoop[ start ];
		face->edgeLoop[ start ]  = face->edgeLoop[ end ];
		face->edgeLoop[ end ]    = temp;

		start++;
		end--;
	}

	compute_brush_face_normal( face );

	ApeBrush *brush = face->parent;
	ApeRoom  *room  = ape_world_node_get_room( APE_WORLD_NODE( brush ) );
	if ( room != nullptr )
	{
		room->isDirty = true;
	}
}

bool ape_brush_build_from_polygon_( ApeBrush *self, const PLVector3 *vertices, uint numVertices, PLVector3 dir, float scale, float signedArea, ApeMaterial *material )
{
	// extrude and build the brush geometry from the given polygon shape
	if ( numVertices < 3 )
	{
		ape_warning_( "Invalid number of vertices for building brush from polygon (%u < 3)!\n", numVertices );
		return false;
	}

	self->numVertices = numVertices * 2;
	self->vertices    = PL_NEW_( PLVector3, self->numVertices );
	memcpy( self->vertices, vertices, numVertices * sizeof( PLVector3 ) );

	self->numFaces = numVertices + 2;// plus two for top and bottom
	self->faces    = PL_NEW_( ApeBrushFace, self->numFaces );

	// set up the top and bottom faces first
	self->faces[ 0 ].numVertices = self->faces[ 1 ].numVertices = numVertices;
	for ( uint i = 0; i < numVertices; ++i )
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

		uint next = ( i + 1 ) % numVertices;

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

		for ( uint j = 0; j < 4; ++j )
		{
			self->faces[ i + 2 ].edgeLoop[ j ] = ( signedArea < 0.0f ) ? &self->faces[ i + 2 ].vertices[ j ] : &self->faces[ i + 2 ].vertices[ 3 - j ];
		}
	}

	for ( uint i = 0; i < self->numFaces; ++i )
	{
		self->faces[ i ].parent = self;
		self->faces[ i ].colour = PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.0f );

		self->faces[ i ].material      = material;
		self->faces[ i ].materialScale = PL_VECTOR2( 0.5f, 0.5f );

		compute_brush_face_normal( &self->faces[ i ] );
		compute_brush_face_bounds( &self->faces[ i ] );
		compute_brush_face_texture_coordinates( &self->faces[ i ] );
		compute_brush_face_tangents( &self->faces[ i ] );
	}

	compute_brush_bounds( self );

	ApeRoom *room = ape_world_node_get_room( &self->base );
	if ( room != nullptr )
	{
		room->isDirty = true;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////

bool ape_world_face_is_mirror( const ApeWorldFace *self )
{
	uint flags = ape_material_get_flags( self->material );
	if ( flags & APE_MATERIAL_FLAG_MIRROR )
	{
		return true;
	}

	return ( self->flags & APE_WORLD_FACE_FLAG_MIRRORED );
}

bool ape_world_face_is_portal( const ApeWorldFace *self )
{
	return ( ape_world_face_is_mirror( self ) || ( self->portal != NULL ) );
}

AcmBranch *ape_brush_serialize_( void *self, AcmBranch *root )
{
	ApeBrush *brush = ( ApeBrush * ) self;
	acm_push_ui32( root, "type", brush->type );
	acm_push_array_f32( root, "vertices", ( float * ) brush->vertices, brush->numVertices * 3 );

	AcmBranch *facesBranch = acm_push_array_object( root, "faces" );
	for ( uint i = 0; i < brush->numFaces; ++i )
	{
		const ApeBrushFace *face       = &brush->faces[ i ];
		AcmBranch          *faceBranch = acm_push_object( facesBranch, "face" );

		acm_push_string( faceBranch, "id", face->id, true );
		if ( face->destination != nullptr )
		{
			assert( *face->destination->id != '\0' );
			acm_push_string( faceBranch, "destination", face->destination->id, false );
		}

		assert( face->material != nullptr );
		acm_push_string( faceBranch, "material", ape_material_get_path( face->material ), false );
		acm_push_vector2( faceBranch, "materialScale", &face->materialScale, true );
		acm_push_vector3( faceBranch, "materialOffset", &face->materialOffset, true );
		acm_push_vector3( faceBranch, "materialAngle", &face->materialAngle, true );

		acm_push_vector3( faceBranch, "normal", &face->normal, true );
		acm_push_array_f32( faceBranch, "colour", ( float * ) &face->colour, 4 );
		acm_push_array_f32( faceBranch, "bounds", ( float * ) &face->bounds, 12 );

		acm_push_ui32( faceBranch, "flags", face->flags );

		AcmBranch *edgeBranch     = acm_push_array_i16( faceBranch, "edgeLoop", nullptr, 0 );
		AcmBranch *verticesBranch = acm_push_array_object( faceBranch, "vertices" );
		for ( uint j = 0; j < face->numVertices; ++j )
		{
			const ApeBrushFaceVertex *vertex       = &face->vertices[ j ];
			AcmBranch                *vertexBranch = acm_push_object( verticesBranch, "vertex" );
			acm_push_i16( vertexBranch, "position", vertex->position - brush->vertices );
			acm_push_vector2( vertexBranch, "uv", &vertex->textureCoords, true );
			acm_push_vector3( vertexBranch, "normal", &vertex->normal, true );
			acm_push_array_f32( vertexBranch, "colour", ( float * ) &vertex->colour, 4 );

			acm_push_i16( edgeBranch, "vertex", face->edgeLoop[ j ] - face->vertices );
		}
	}

	return root;
}

ApeWorldNode *ape_brush_deserialize_( ApeWorldNode *parent, AcmBranch *root )
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
		for ( uint i = 0; i < self->numFaces; ++i, branch = acm_get_next_child( branch ) )
		{
			self->faces[ i ].parent = self;

			AcmBranch *vertexBranch = acm_get_child_by_name( branch, "vertices" );
			if ( vertexBranch != nullptr )
			{
				self->faces[ i ].numVertices = acm_get_num_of_children( vertexBranch );

				vertexBranch = acm_get_first_child( vertexBranch );
				for ( uint j = 0; j < self->faces[ i ].numVertices; ++j, vertexBranch = acm_get_next_child( vertexBranch ) )
				{
					int16_t vertexIndex = ACM_GET_INT( vertexIndex, vertexBranch, "position", 0 );
					assert( vertexIndex <= self->numVertices );
					self->faces[ i ].vertices[ j ].position      = &self->vertices[ vertexIndex ];
					self->faces[ i ].vertices[ j ].textureCoords = acm_get_vector2( vertexBranch, "uv", &( PLVector2 ) {} );
					self->faces[ i ].vertices[ j ].normal        = acm_get_vector3( vertexBranch, "normal", &( PLVector3 ) {} );
					self->faces[ i ].vertices[ j ].colour        = acm_get_colour_f32( vertexBranch, "colour", &( PLColourF32 ) { .a = 1.0f } );
				}

				int16_t edgeLoop[ APE_BRUSH_MAX_FACE_VERTICES ];
				acm_get_array_i16( branch, "edgeLoop", edgeLoop, self->faces[ i ].numVertices );
				for ( uint j = 0; j < self->faces[ i ].numVertices; ++j )
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

			snprintf( self->faces[ i ].id, sizeof( self->faces[ i ].id ), "%s", acm_get_string( branch, "id", "" ) );

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
			self->faces[ i ].materialScale  = acm_get_vector2( branch, "materialScale", &( PLVector2 ) {} );
			self->faces[ i ].materialOffset = acm_get_vector3( branch, "materialOffset", &( PLVector3 ) {} );
			self->faces[ i ].materialAngle  = acm_get_vector3( branch, "materialAngle", &( PLVector3 ) {} );

			self->faces[ i ].flags = ACM_GET_UINT( self->faces[ i ].flags, branch, "flags", 0 );

			self->faces[ i ].normal = acm_get_vector3( branch, "normal", &( PLVector3 ) {} );
			self->faces[ i ].colour = acm_get_colour_f32( branch, "colour", &( PLColourF32 ) { .a = 1.0f } );
			acm_get_array_f32( branch, "bounds", ( float * ) &self->faces[ i ].bounds, 12 );

			compute_brush_face_tangents( &self->faces[ i ] );
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

const ApeWorldNodeClass ape_brushClass = {
        .identifier          = "brush",
        .magic               = PL_MAGIC_TO_NUM( 'B', 'R', 'S', 'H' ),
        .destroyFunction     = ape_brush_destroy_,
        .serializeFunction   = ape_brush_serialize_,
        .deserializeFunction = ape_brush_deserialize_,
};
