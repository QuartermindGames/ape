// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Implementation of the world building blocks - brushes.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "world/world.h"
#include "client/renderer/material/material.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

//TODO: eventually we should do away with this
#define MAX_MATERIALS_PER_PASS 256
static int subMeshes[ MAX_MATERIALS_PER_PASS ][ APE_BRUSH_MAX_SUB_MESHES ];
static int firstSubMeshes[ MAX_MATERIALS_PER_PASS ][ APE_BRUSH_MAX_SUB_MESHES ];
static int numSubMeshes[ MAX_MATERIALS_PER_PASS ];

#define SELF( X ) APE_SELF_CAST( ApePolyBrush, X )

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
		ApeWorldFace *face = PlGetVectorArrayElementAt( self->faces, j );
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
			                                               &vertex->position,
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

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeBrush *ape_create_brush( ApeWorldNode *parent, const PLVector3 *position, const PLVector3 *angles )
{
	ApeBrush *brush = PL_NEW( ApeBrush );
	ape_world_node_setup_( &brush->base, parent, APE_WORLD_NODE_TYPE_BRUSH, nullptr, position, angles );
	return brush;
}

void ape_brush_destroy_( void *data )
{
	ApeBrush *self = ( ApeBrush * ) data;
	if ( self == nullptr )
	{
		return;
	}

	PlgDestroyMesh( self->mesh );

	PL_DELETE( self );
}

void ape_brush_node_draw_( void *data, const PLMatrix4 *transform )
{
	ApeBrush *self = ( ApeBrush * ) data;
	assert( self != nullptr );

	// It's assumed all vis checks were done beforehand...

	upload_mesh( self );

	draw_faces( self );
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
