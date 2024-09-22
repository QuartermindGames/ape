// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "world/world.h"
#include "client/renderer/material/material.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

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

		PLVector3 edge1 = PLVector3( next->x - current->x, next->y - current->y, next->z - current->z );
		PLVector3 edge2 = PLVector3( prev->x - current->x, prev->y - current->y, prev->z - current->z );

		PLVector3 n  = PlVector3CrossProduct( edge1, edge2 );
		face->normal = PlAddVector3( face->normal, n );
	}
	face->normal = PlNormalizeVector3( face->normal );
}

bool ape_brush_build_from_polygon_( ApeBrush *self, const PLVector3 *vertices, uint numVertices, PLVector3 dir, float scale )
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
		// bottom
		self->faces[ 0 ].vertices[ i ].position = &self->vertices[ i ];
		self->faces[ 0 ].edgeLoop[ i ]          = &self->faces[ 0 ].vertices[ i ];
		// top (edge loop reversed)
		self->faces[ 1 ].vertices[ i ].position          = &self->vertices[ i + numVertices ];
		self->faces[ 1 ].edgeLoop[ numVertices - 1 - i ] = &self->faces[ 1 ].vertices[ i ];
		// extrude the vertices for the top
		self->vertices[ i + numVertices ] = PlSubtractVector3( self->vertices[ i ], PlScaleVector3F( dir, scale ) );
	}

	compute_brush_face_normal( &self->faces[ 0 ] );
	compute_brush_face_normal( &self->faces[ 1 ] );

	// set up the faces for the edges
	for ( uint i = 0; i < numVertices; ++i )
	{
		uint next = ( i + 1 ) % numVertices;

		PLVector3 *a = &self->vertices[ i ];
		PLVector3 *b = &self->vertices[ next ];
		PLVector3 *c = &self->vertices[ i + numVertices ];
		PLVector3 *d = &self->vertices[ next + numVertices ];

		self->faces[ i + 2 ].numVertices            = 4;
		self->faces[ i + 2 ].vertices[ 0 ].position = a;
		self->faces[ i + 2 ].vertices[ 1 ].position = b;
		self->faces[ i + 2 ].vertices[ 2 ].position = d;
		self->faces[ i + 2 ].vertices[ 3 ].position = c;

		self->faces[ i + 2 ].edgeLoop[ 0 ] = &self->faces[ i + 2 ].vertices[ 0 ];
		self->faces[ i + 2 ].edgeLoop[ 1 ] = &self->faces[ i + 2 ].vertices[ 1 ];
		self->faces[ i + 2 ].edgeLoop[ 2 ] = &self->faces[ i + 2 ].vertices[ 2 ];
		self->faces[ i + 2 ].edgeLoop[ 3 ] = &self->faces[ i + 2 ].vertices[ 3 ];

		compute_brush_face_normal( &self->faces[ i + 2 ] );
	}

#if 1
	for ( uint i = 0; i < self->numFaces; ++i )
	{
		self->faces[ i ].colour = PL_COLOURF32( PlGenerateRandomFloat( 1.0f ),
		                                        PlGenerateRandomFloat( 1.0f ),
		                                        PlGenerateRandomFloat( 1.0f ), 255 );
	}
#endif

	return true;
}

void ape_brush_node_draw_( void *data, const PLMatrix4 *transform )
{
	ApeBrush *self = ( ApeBrush * ) data;
	assert( self != nullptr );

	//FIXME: temporary code!!! this should use the display list crap...
	for ( uint i = 0; i < self->numFaces; ++i )
	{
		PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
		for ( uint j = 0; j < self->faces[ i ].numVertices; ++j )
		{
			PlgImmPushVertex( self->faces[ i ].edgeLoop[ j ]->position->x,
			                  self->faces[ i ].edgeLoop[ j ]->position->y,
			                  self->faces[ i ].edgeLoop[ j ]->position->z );
			PlgImmColour( PlFloatToByte( self->faces[ i ].colour.r ),
			              PlFloatToByte( self->faces[ i ].colour.g ),
			              PlFloatToByte( self->faces[ i ].colour.b ), 255 );
		}

		ApeMaterial *material = ss_arl_get_default_material( SS_ARL_MATERIAL_DEFAULT_VERTEX );
		assert( material != nullptr );
		ape_material_draw( material, mesh, nullptr );
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, self->base.children, i )
	{
		ape_brush_node_draw_( child, nullptr );
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
