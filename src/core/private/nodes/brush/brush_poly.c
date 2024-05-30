// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Default poly brush class.
// Author:  Mark E. Sowden

#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

//TODO: eventually we should do away with this
#define MAX_MATERIALS_PER_PASS 256
#define MAX_SUB_MESHES         8192
static int subMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int firstSubMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int numSubMeshes[ MAX_MATERIALS_PER_PASS ];

typedef struct ApePolyBrushFaceVertex
{
	PLVector2 textureCoords;
	PLVector2 lightmapCoords;
	PLVector3 position;
	PLVector3 normal;
	PLColourF32 colour;
} ApePolyBrushFaceVertex;

typedef struct ApePolyBrushFace
{
	int materialIndex;

	PLVectorArray *vertices;//ApePolyBrushFaceVertex
	PLLinkedList *edgeLoop; //ApePolyBrushFaceVertex

	unsigned int flags;

	struct ApePolyBrushFace *connectedPortalFace;
} ApePolyBrushFace;

typedef struct ApePolyBrush
{
	ApeMaterial *materials[ MAX_SUB_MESHES ];

	PLVectorArray *faces;//ApePolyBrushFace

	PLGMesh *mesh;    // cached mesh
	bool isMeshCached;// if false, mesh cache will be updated
} ApePolyBrush;

#define SELF( X ) APE_SELF_CAST( ApePolyBrush, X )

static unsigned int get_total_verts( const ApePolyBrush *self )
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

static unsigned int get_total_faces( ApePolyBrush *self )
{
	return PlGetNumVectorArrayElements( self->faces );
}

static ApePolyBrushFace **get_faces( ApePolyBrush *self, unsigned int *num )
{
	return ( ApePolyBrushFace ** ) PlGetVectorArrayDataEx( self->faces, num );
}

static void upload_mesh( ApePolyBrush *self )
{
	if ( SELF( self )->isMeshCached )
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
			ApePolyBrushFaceVertex *vertex = PlGetLinkedListNodeUserData( faceVertexNode );
			unsigned int v = PlgAddMeshVertex( self->mesh,
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

static void draw_faces( ApePolyBrush *self )
{
	unsigned int numFaces;
	ApePolyBrushFace **faces = get_faces( self, &numFaces );
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

		assert( numSubMeshes[ faces[ i ]->materialIndex ] < MAX_SUB_MESHES );
		if ( numSubMeshes[ faces[ i ]->materialIndex ] >= MAX_SUB_MESHES )
		{
			PRINT_WARNING( "Hit submesh limit for draw, will squeeze into another batch!\n" );
			break;
		}
	}
}

static void poly_brush_draw( ApeBrush *self )
{
	// It's assumed all vis checks were done beforehand...

	upload_mesh( SELF( self ) );

	draw_faces( SELF( self ) );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeBrushClass ape_polyBrushClass = {
        .name = "polyBrushClass",
        .editorName = "Poly Brush",
        .editorDescription = "Basic brush used for building polygonal geometry.",

        .drawFunction = poly_brush_draw,
};
