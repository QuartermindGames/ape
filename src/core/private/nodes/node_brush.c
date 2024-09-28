// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "world/world.h"
#include "client/renderer/material/material.h"

static constexpr uint BRUSH_MAX_FACE_TRIANGLES = ( APE_BRUSH_MAX_FACE_VERTICES - 3 );
static constexpr uint BRUSH_MAX_FACE_INDICES   = BRUSH_MAX_FACE_TRIANGLES * 3;

#if 0
//TODO: eventually we should do away with this
#	define MAX_MATERIALS_PER_PASS 256
static int subMeshes[ MAX_MATERIALS_PER_PASS ][ APE_BRUSH_MAX_SUB_MESHES ];
static int firstSubMeshes[ MAX_MATERIALS_PER_PASS ][ APE_BRUSH_MAX_SUB_MESHES ];
static int numSubMeshes[ MAX_MATERIALS_PER_PASS ];

#	define SELF( X ) APE_SELF_CAST( ApePolyBrush, X )

static unsigned int get_total_verts( const ApeBrush *self )
{
	// determine the total number of vertices

	unsigned int numVerts = 0;
	for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( self->faces ); ++j )
	{
		ApeWorldFace *face = PlGetVectorArrayElementAt( self->faces, j );
		assert( face != nullptr );
		if ( face == nullptr )
		{
			continue;
		}

		numVerts += PlGetNumLinkedListNodes( face->edgeLoop );
	}

	return numVerts;
}

static unsigned int get_total_faces( ApeBrush *self )
{
	return PlGetNumVectorArrayElements( self->faces );
}

static ApeBrushFace **get_faces( ApeBrush *self, unsigned int *num )
{
	return ( ApeBrushFace ** ) PlGetVectorArrayDataEx( self->faces, num );
}

static void upload_mesh( ApeBrush *self )
{
	if ( self->isMeshCached )
	{
		return;
	}

	if ( self->mesh == nullptr )
	{
		self->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_FAN, PLG_DRAW_STATIC, 0, get_total_verts( self ) );
		if ( self->mesh == nullptr )
		{
			ape_warning_( "Failed to create mesh for room: %s\n", PlGetError() );
			return;
		}
	}

	PlgClearMesh( self->mesh );

	unsigned int numFaces = get_total_faces( self );
	for ( unsigned int j = 0; j < numFaces; ++j )
	{
		ApeWorldFace *face = &self->faces[ j ];
		assert( face != nullptr );
		if ( face->materialIndex < 0 )
		{
			continue;
		}

		PLColour faceColour = {
		        .r = ( rand() % 200 ) + 55,
		        .g = ( rand() % 200 ) + 55,
		        .b = ( rand() % 200 ) + 55,
		        .a = 255,
		};

		PLLinkedListNode *faceVertexNode = PlGetFirstNode( face->edgeLoop );
		while ( faceVertexNode != nullptr )
		{
			ApeBrushFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
			unsigned int        v      = PlgAddMeshVertex( self->mesh,
			                                               vertex->position,
			                                               &vertex->normal,
			                                               &faceColour,
			                                               &vertex->textureCoords );
			PlgSetMeshVertexSTv( self->mesh, 1, v, 2, ( float * ) &vertex->lightmapCoords );

			faceVertexNode = PlGetNextLinkedListNode( faceVertexNode );
		}
	}

	PlgGenerateVertexTangentBasis( self->mesh->vertices, self->mesh->num_verts );
	PlgUploadMesh( self->mesh );

	self->isMeshCached = true;
}

static void draw_faces( ApeBrush *self )
{
	unsigned int   numFaces;
	ApeBrushFace **faces = get_faces( self, &numFaces );
	if ( numFaces == 0 )
	{
		return;
	}

	unsigned int numVertices;
	for ( unsigned int i = 0, offset = 0; i < numFaces; ++i, offset += numVertices )
	{
		numVertices = PlGetNumLinkedListNodes( faces[ i ]->edgeLoop );

		if ( faces[ i ]->materialIndex < 0 )
		{
			continue;
		}

		assert( numSubMeshes[ faces[ i ]->materialIndex ] < APE_BRUSH_MAX_SUB_MESHES );
		if ( numSubMeshes[ faces[ i ]->materialIndex ] >= APE_BRUSH_MAX_SUB_MESHES )
		{
			PRINT_WARNING( "Hit submesh limit for draw, will squeeze into another batch!\n" );
			break;
		}
	}
}
#endif

ApeBrush *ape_brush_create( ApeWorldNode *parent, const char *name, const PLVector3 *position, const PLVector3 *angles )
{
	ApeBrush *brush = PL_NEW( ApeBrush );
	if ( ape_world_node_setup_( &brush->base, parent, APE_WORLD_NODE_TYPE_BRUSH, name, position, angles ) == nullptr )
	{
		PL_DELETEN( brush );
	}

	return brush;
}

void ape_brush_destroy_( void *data )
{
	ApeBrush *self = ( ApeBrush * ) data;
	if ( self == nullptr )
	{
		return;
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
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		unsigned int j = ( i + 1 ) % face->numVertices;// next vertex index (wraps around)

		const PLVector3 *current = face->vertices[ i ].position;
		const PLVector3 *next    = face->vertices[ j ].position;
		const PLVector3 *prev    = face->vertices[ ( i == 0 ) ? face->numVertices - 1 : ( i - 1 ) ].position;

		PLVector3 edge1 = PL_VECTOR3( next->x - current->x, next->y - current->y, next->z - current->z );
		PLVector3 edge2 = PL_VECTOR3( prev->x - current->x, prev->y - current->y, prev->z - current->z );

		PLVector3 n  = PlVector3CrossProduct( edge1, edge2 );
		face->normal = PlAddVector3( face->normal, n );
	}
	face->normal = PlNormalizeVector3( face->normal );
}

static void compute_brush_face_texture_coordinates( ApeBrushFace *face )
{
	for ( unsigned int i = 0, x, y; i < face->numVertices; ++i )
	{
		if ( ( fabsf( face->normal.x ) > fabsf( face->normal.y ) ) &&
		     ( fabsf( face->normal.x ) > fabsf( face->normal.z ) ) )
		{
			x = ( face->normal.x > 0.0 ) ? 1 : 2;
			y = ( face->normal.x > 0.0 ) ? 2 : 1;
		}
		else if ( ( fabsf( face->normal.z ) > fabsf( face->normal.x ) ) &&
		          ( fabsf( face->normal.z ) > fabsf( face->normal.y ) ) )
		{
			x = ( face->normal.z > 0.0 ) ? 0 : 1;
			y = ( face->normal.z > 0.0 ) ? 1 : 0;
		}
		else
		{
			x = ( face->normal.y > 0.0 ) ? 2 : 0;
			y = ( face->normal.y > 0.0 ) ? 0 : 2;
		}

		face->edgeLoop[ i ]->textureCoords.x = ( PL_VECTOR3_I( *face->edgeLoop[ i ]->position, x ) + face->materialOffset.x ) * face->materialScale.x;
		face->edgeLoop[ i ]->textureCoords.y = ( PL_VECTOR3_I( *face->edgeLoop[ i ]->position, y ) + face->materialOffset.y ) * face->materialScale.y;
	}
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
}

bool ape_brush_build_from_polygon_( ApeBrush *self, const PLVector3 *vertices, uint numVertices, PLVector3 dir, float scale )
{
	// extrude and build the brush geometry from the given polygon shape
	if ( numVertices < 3 )
	{
		ape_warning_( "Invalid number of vertices for building brush from polygon (%u < 3)!\n", numVertices );
		return false;
	}

	// use this to determine the order, so we can reverse for edge loop if needed
	// todo: leave this to the caller, not here!!
	float signedArea = 0.0f;
	for ( uint i = 0; i < numVertices; ++i )
	{
		uint next = ( i + 1 ) % numVertices;
		signedArea += ( vertices[ i ].x * vertices[ next ].z - vertices[ next ].x * vertices[ i ].z );
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

		self->faces[ i + 2 ].materialScale = PL_VECTOR3( 1.0f, 1.0f, 1.0f );
	}

	for ( uint i = 0; i < self->numFaces; ++i )
	{
		self->faces[ i ].colour   = PL_COLOURF32( PlGenerateRandomFloat( 1.0f ),
		                                          PlGenerateRandomFloat( 1.0f ),
		                                          PlGenerateRandomFloat( 1.0f ), 255 );
		self->faces[ i ].material = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR );

		compute_brush_face_normal( &self->faces[ i ] );
		compute_brush_face_texture_coordinates( &self->faces[ i ] );
	}

	compute_brush_bounds( self );

	return true;
}

void ape_brush_node_draw_( ApeBrush *self, ApeCameraDrawMode drawMode, const PLMatrix4 *transform )
{
	ApeEditorInstance *editorInstance = ape_editor_get_active_instance();

	//FIXME: temporary code!!! this should use the display list crap...
	for ( uint i = 0; i < self->numFaces; ++i )
	{
		if ( self->faces[ i ].material == nullptr )
		{
			continue;
		}

#if 0// we can use this when concave polys are supported

		uint indices[ BRUSH_MAX_FACE_INDICES ] = {};

		uint numTriangles = convert_brush_polygon_to_triangles( &self->faces[ i ], indices );
		assert( numTriangles > 0 );

		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLES );
		for ( uint j = 0; j < self->faces[ i ].numVertices; ++j )
		{
			const ApeBrushFaceVertex *vertex = &self->faces[ i ].vertices[ j ];
			PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
			PlgImmNormal( self->faces[ i ].normal.x, self->faces[ i ].normal.y, self->faces[ i ].normal.z );
			PlgImmTextureCoord( vertex->textureCoords.x, vertex->textureCoords.y );
			PlgImmColour( PlFloatToByte( self->faces[ i ].colour.r ),
			              PlFloatToByte( self->faces[ i ].colour.g ),
			              PlFloatToByte( self->faces[ i ].colour.b ), 255 );
		}

		for ( uint j = 0; j < ( numTriangles * 3 ); j += 3 )
		{
			PlgImmPushTriangle( indices[ j ], indices[ j + 1 ], indices[ j + 2 ] );
		}

#else// but alas, for now...

		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
		for ( uint j = 0; j < self->faces[ i ].numVertices; ++j )
		{
			const ApeBrushFaceVertex *vertex = self->faces[ i ].edgeLoop[ j ];
			PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
			PlgImmNormal( self->faces[ i ].normal.x, self->faces[ i ].normal.y, self->faces[ i ].normal.z );
			PlgImmTextureCoord( vertex->textureCoords.x, vertex->textureCoords.y );
			PlgImmColour( PlFloatToByte( self->faces[ i ].colour.r ),
			              PlFloatToByte( self->faces[ i ].colour.g ),
			              PlFloatToByte( self->faces[ i ].colour.b ), 255 );
		}

#endif

		ape_material_draw( self->faces[ i ].material, mesh, nullptr );
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->base.children, i )
	{
		if ( child->type != APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		ape_brush_node_draw_( ( ApeBrush * ) child, APE_CAMERA_DRAW_MODE_INVALID, nullptr );
	}
}

/////////////////////////////////////////////////////////////////////////////////////

bool ape_world_face_is_mirror( const ApeWorldFace *self )
{
	unsigned int flags = ape_material_get_flags( self->material );
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
