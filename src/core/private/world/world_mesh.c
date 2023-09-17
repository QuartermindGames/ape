// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "ape_private.h"
#include "world.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_material.h"

#define WORLD_VERTEX_ELEMENTS 12// pos, norm, uv, colour

unsigned int *apeConvertWorldFaceToTriangles( const ApeWorldFace *face, unsigned int *numTriangles ) {
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

static void DeserializeMaterials( NdBranch *meshNode, ApeWorldMesh *meshPtr ) {
	NdBranch *materialsList = ndGetChildByName( meshNode, "materials" );
	if ( materialsList == NULL ) {
		PRINT_WARNING( "No materials for mesh: %s!\n", meshPtr->id );
		return;
	}

	meshPtr->numMaterials = ndGetNumOfChildren( materialsList );
	meshPtr->materials = PlCAlloc( meshPtr->numMaterials, sizeof( ApeMaterial * ), true );
	NdBranch *materialNode = ndGetFirstChild( materialsList );
	for ( unsigned int i = 0; i < meshPtr->numMaterials; ++i ) {
		if ( materialNode == NULL ) {
			PRINT_WARNING( "Hit an invalid material index!\n" );
			meshPtr->numMaterials = i;
			break;
		}

		char materialPath[ PL_SYSTEM_MAX_PATH ];
		ndGetStr( materialNode, materialPath, sizeof( materialPath ) );
		meshPtr->materials[ i ] = apeCacheMaterial( materialPath, APE_CACHE_WORLD, true, false );
		materialNode = ndGetNextChild( materialNode );
	}
}

static ApeWorldVertex *DeserializeVertices( NdBranch *meshNode, unsigned int *numVertices ) {
	NdBranch *verticesList = ndGetChildByName( meshNode, "vertices" );
	if ( verticesList == NULL )
		return NULL;

	unsigned int numChildren = ndGetNumOfChildren( verticesList );
	float *data = PL_NEW_( float, numChildren );
	if ( ndGetF32Array( verticesList, ( float * ) data, numChildren ) != ND_ERROR_SUCCESS ) {
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
ApeWorldMesh *YnCore_WorldDeserialiser_BeginMesh( NdBranch *root, ApeWorldMesh *worldMesh ) {
	DeserializeMaterials( root, worldMesh );

	unsigned int numVertices;
	ApeWorldVertex *vertices = DeserializeVertices( root, &numVertices );
	if ( vertices == NULL ) {
		PRINT_WARNING( "Failed to fetch vertices for mesh: %s\n", worldMesh->id );
		return NULL;
	}
	worldMesh->maxVertices = worldMesh->numVertices = numVertices;
	worldMesh->vertices = vertices;

	return worldMesh;
}

static void GenerateBounds( ApeWorldMesh *mesh ) {
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
void DestroyWorldMesh( ApeWorldMesh *mesh ) {
	PlgDestroyMesh( mesh->drawMesh );
}

ApeWorldMesh *apeCreateWorldMesh( ApeWorld *parent ) {
	ApeWorldMesh *mesh = PL_NEW( ApeWorldMesh );
	mesh->faces = PlCreateLinkedList();

	if ( parent != NULL )
		PlPushBackVectorArrayElement( parent->meshes, mesh );

	apeSetupReference( "WorldMesh", APE_CACHE_POOL_WORLD_MESHES, &mesh->mem, ( MMReference_CleanupFunction ) DestroyWorldMesh, mesh );
	apeAddReference( &mesh->mem );

	return mesh;
}
