// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "ape_private.h"
#include "world.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_material.h"

#define WORLD_VERTEX_ELEMENTS 12// pos, norm, uv, colour

bool YnCore_World_IsFaceVisible( ApeWorldFace *face, const ApeCamera *camera )
{
	// Check the face is actually visible
	face->bounds.origin = pl_vecOrigin3;
	if ( !PlgIsBoxInsideView( camera->internal, &face->bounds ) )
		return false;

	return true;
}

unsigned int *apeConvertWorldFaceToTriangles( const ApeWorldFace *face, unsigned int *numTriangles )
{
#if 0
	if ( face->numVertices < 3 )
		return NULL;

	*numTriangles = face->numVertices - 2;
	if ( *numTriangles == 0 )
		return NULL;

	unsigned int *indices = PlMAllocA( sizeof( unsigned int ) * ( *numTriangles * 3 ) );
	unsigned int *index   = indices;
	for ( unsigned int i = 1; i + 1 < face->numVertices; ++i, index += 3 )
	{
		index[ 0 ] = face->vertices[ 0 ];
		index[ 1 ] = face->vertices[ i ];
		index[ 2 ] = face->vertices[ i + 1 ];
	}

	return indices;
#else
	return NULL;
#endif
}

static void GenerateFaceNormal( const ApeWorldMesh *mesh, ApeWorldFace *face )
{
#if 0
	for ( unsigned int i = 0; i < face->numVertices; ++i )
		face->normal = PlAddVector3( face->normal, mesh->vertices[ face->vertices[ i ] ].normal );

	face->normal = PlNormalizeVector3( face->normal );
#endif
}

static void DeserializeMaterials( NdBranch *meshNode, ApeWorldMesh *meshPtr )
{
	NdBranch *materialsList = ndGetChildByName( meshNode, "materials" );
	if ( materialsList == NULL )
	{
		PRINT_WARNING( "No materials for mesh: %s!\n", meshPtr->id );
		return;
	}

	meshPtr->numMaterials  = ndGetNumOfChildren( materialsList );
	meshPtr->materials     = PlCAlloc( meshPtr->numMaterials, sizeof( ApeMaterial     *), true );
	NdBranch *materialNode = ndGetFirstChild( materialsList );
	for ( unsigned int i = 0; i < meshPtr->numMaterials; ++i )
	{
		if ( materialNode == NULL )
		{
			PRINT_WARNING( "Hit an invalid material index!\n" );
			meshPtr->numMaterials = i;
			break;
		}

		char materialPath[ PL_SYSTEM_MAX_PATH ];
		ndGetStr( materialNode, materialPath, sizeof( materialPath ) );
		meshPtr->materials[ i ] = apeCacheMaterial( materialPath, APE_CACHE_WORLD, true, false );
		materialNode            = ndGetNextChild( materialNode );
	}
}

static ApeWorldVertex *DeserializeVertices( NdBranch *meshNode, unsigned int *numVertices )
{
	NdBranch *verticesList = ndGetChildByName( meshNode, "vertices" );
	if ( verticesList == NULL )
		return NULL;

	unsigned int numChildren = ndGetNumOfChildren( verticesList );
	float *data              = PL_NEW_( float, numChildren );
	if ( ndGetF32Array( verticesList, ( float * ) data, numChildren ) != ND_ERROR_SUCCESS )
	{
		PRINT_WARNING( "Failed to fetch all vertices for mesh!\n" );
		PL_DELETE( data );
		return NULL;
	}

	*numVertices = numChildren / sizeof( ApeWorldVertex );
	return ( ApeWorldVertex * ) data;
}

/**
 * Deserialise a mesh from the given node.
 */
ApeWorldMesh *YnCore_WorldDeserialiser_BeginMesh( NdBranch *root, ApeWorldMesh *worldMesh )
{
	DeserializeMaterials( root, worldMesh );

	unsigned int numVertices;
	ApeWorldVertex *vertices = DeserializeVertices( root, &numVertices );
	if ( vertices == NULL )
	{
		PRINT_WARNING( "Failed to fetch vertices for mesh: %s\n", worldMesh->id );
		return NULL;
	}
	worldMesh->maxVertices = worldMesh->numVertices = numVertices;
	worldMesh->vertices                             = vertices;

	return worldMesh;
}

static void GenerateBounds( ApeWorldMesh *mesh )
{
	PLVector3 *coords = PL_NEW_( PLVector3, mesh->numVertices );
	for ( unsigned int i = 0; i < mesh->numVertices; ++i )
		coords[ i ] = mesh->vertices[ i ].position;

	mesh->bounds = PlGenerateAabbFromCoords( coords, mesh->numVertices, true );
	PL_DELETE( coords );

#if 0
	PLLinkedListNode *faceNode = PlGetFirstNode( mesh->faces );
	while ( faceNode != NULL )
	{
		OgeWorldFace *face = PlGetLinkedListNodeUserData( faceNode );
		coords                = PL_NEW_( PLVector3, face->numVertices );
		for ( unsigned int j = 0; j < face->numVertices; ++j )
			coords[ j ] = mesh->vertices[ face->vertices[ j ] ].position;

		face->bounds = PlGenerateAabbFromCoords( coords, face->numVertices, true );
		PL_DELETE( coords );

		face->origin = face->bounds.absOrigin;

		faceNode = PlGetNextLinkedListNode( faceNode );
	}
#endif
}

/**
 * Free the mesh from memory.
 */
void DestroyWorldMesh( ApeWorldMesh *mesh )
{
	PlgDestroyMesh( mesh->drawMesh );
}

ApeWorldMesh *apeCreateWorldMesh( ApeWorld *parent )
{
	ApeWorldMesh *mesh = PL_NEW( ApeWorldMesh );
	mesh->faces        = PlCreateLinkedList();

	if ( parent != NULL )
		PlPushBackVectorArrayElement( parent->meshes, mesh );

	apeSetupReference( "WorldMesh", APE_CACHE_POOL_WORLD_MESHES, &mesh->mem, ( MMReference_CleanupFunction ) DestroyWorldMesh, mesh );
	apeAddReference( &mesh->mem );

	return mesh;
}

ApeWorldMesh *apeLoadWorldMesh( const char *path )
{
	// Check to see if it's cached already
	ApeWorldMesh *worldMesh = apeGetCachedData( path, APE_CACHE_POOL_WORLD_MESHES );
	if ( worldMesh != NULL )
	{
		apeAddReference( &worldMesh->mem );
		return worldMesh;
	}

	NdBranch *node = ndLoadFile( path, "worldMesh" );
	if ( node == NULL )
	{
		PRINT_WARNING( "Failed to load world mesh: %s\n", path );
		return NULL;
	}

	worldMesh = apeCreateWorldMesh( NULL );
	if ( YnCore_WorldDeserialiser_BeginMesh( node, worldMesh ) == NULL )
	{
		apeReleaseWorldMesh( worldMesh );
		worldMesh = NULL;
	}

	ndDestroyBranch( node );

	// If it loaded fine, be sure we start tracking it
	if ( worldMesh != NULL )
	{
		GenerateBounds( worldMesh );

		worldMesh->drawMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, PlGetNumLinkedListNodes( worldMesh->faces ), worldMesh->numVertices );
		if ( worldMesh->drawMesh == NULL )
			PRINT_ERROR( "Failed to create internal mesh for world mesh!\n" );

		// Push all the vertices to the mesh
		for ( unsigned int i = 0; i < worldMesh->numVertices; ++i )
		{
			PLColour colour = PlColourF32ToU8( &worldMesh->vertices[ i ].colour );
			PlgAddMeshVertex( worldMesh->drawMesh,
			                  &worldMesh->vertices[ i ].position,
			                  &worldMesh->vertices[ i ].normal,
			                  &colour,
			                  &worldMesh->vertices[ i ].uv );
		}

		PLLinkedListNode *faceNode = PlGetFirstNode( worldMesh->faces );
		while ( faceNode != NULL )
		{
			ApeWorldFace *face = PlGetLinkedListNodeUserData( faceNode );

			unsigned int numTriangles;
			unsigned int *indices  = apeConvertWorldFaceToTriangles( face, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k, curIndex += 3 )
				PlgAddMeshTriangle( worldMesh->drawMesh, curIndex[ 0 ], curIndex[ 1 ], curIndex[ 2 ] );

			PL_DELETE( indices );

			ape_rendererPerformance_.numFacesDrawn++;

			faceNode = PlGetNextLinkedListNode( faceNode );
		}

		PlgGenerateVertexTangentBasis( worldMesh->drawMesh->vertices, worldMesh->drawMesh->num_verts );
		//PlgGenerateMeshTangentBasis( worldMesh->drawMesh );

		apeAddToCachePool( path, APE_CACHE_POOL_WORLD_MESHES, worldMesh );
	}

	return worldMesh;
}

void apeReleaseWorldMesh( ApeWorldMesh *worldMesh )
{
	if ( worldMesh == NULL )
		return;

	apeReleaseReference( &worldMesh->mem );
}
