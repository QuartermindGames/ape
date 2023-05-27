// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "core_private.h"
#include "world.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_material.h"

#define WORLD_VERTEX_ELEMENTS 12// pos, norm, uv, colour

bool YnCore_World_IsFaceVisible( OgeWorldFace *face, const OgeCamera *camera )
{
	// Check the face is actually visible
	face->bounds.origin = pl_vecOrigin3;
	if ( !PlgIsBoxInsideView( camera->internal, &face->bounds ) )
		return false;

	return true;
}

unsigned int *ogeWorld_ConvertFaceToTriangles( const OgeWorldFace *face, unsigned int *numTriangles )
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

static void GenerateFaceNormal( const OgeWorldMesh *mesh, OgeWorldFace *face )
{
#if 0
	for ( unsigned int i = 0; i < face->numVertices; ++i )
		face->normal = PlAddVector3( face->normal, mesh->vertices[ face->vertices[ i ] ].normal );

	face->normal = PlNormalizeVector3( face->normal );
#endif
}

static void DeserializeMaterials( NdBranch *meshNode, OgeWorldMesh *meshPtr )
{
	NdBranch *materialsList = ndGetChildByName( meshNode, "materials" );
	if ( materialsList == NULL )
	{
		PRINT_WARNING( "No materials for mesh: %s!\n", meshPtr->id );
		return;
	}

	meshPtr->numMaterials      = ndGetNumOfChildren( materialsList );
	meshPtr->materials         = PlCAlloc( meshPtr->numMaterials, sizeof( OgeMaterial         *), true );
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
		meshPtr->materials[ i ] = ogeCacheMaterial( materialPath, YN_CORE_CACHE_GROUP_WORLD, true, false );
		materialNode            = ndGetNextChild( materialNode );
	}
}

static OgeWorldVertex *DeserializeVertices( NdBranch *meshNode, unsigned int *numVertices )
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

	*numVertices = numChildren / sizeof( OgeWorldVertex );
	return ( OgeWorldVertex * ) data;
}

/**
 * Deserialise a mesh from the given node.
 */
OgeWorldMesh *YnCore_WorldDeserialiser_BeginMesh( NdBranch *root, OgeWorldMesh *worldMesh )
{
	DeserializeMaterials( root, worldMesh );

	unsigned int numVertices;
	OgeWorldVertex *vertices = DeserializeVertices( root, &numVertices );
	if ( vertices == NULL )
	{
		PRINT_WARNING( "Failed to fetch vertices for mesh: %s\n", worldMesh->id );
		return NULL;
	}
	worldMesh->maxVertices = worldMesh->numVertices = numVertices;
	worldMesh->vertices                             = vertices;

	return worldMesh;
}

static void GenerateBounds( OgeWorldMesh *mesh )
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
void DestroyWorldMesh( OgeWorldMesh *mesh )
{
	PlgDestroyMesh( mesh->drawMesh );
}

OgeWorldMesh *YnCore_WorldMesh_Create( OgeWorld *parent )
{
	OgeWorldMesh *mesh = PL_NEW( OgeWorldMesh );
	mesh->faces           = PlCreateLinkedList();

	if ( parent != NULL )
		PlPushBackVectorArrayElement( parent->meshes, mesh );

	ogeMemoryManager_SetupReference( "WorldMesh", MEM_CACHE_WORLD_MESH, &mesh->mem, ( MMReference_CleanupFunction ) DestroyWorldMesh, mesh );
	ogeMemoryManager_AddReference( &mesh->mem );

	return mesh;
}

OgeWorldMesh *YnCore_WorldMesh_Load( const char *path )
{
	// Check to see if it's cached already
	OgeWorldMesh *worldMesh = ogeMM_GetCachedData( path, MEM_CACHE_WORLD_MESH );
	if ( worldMesh != NULL )
	{
		ogeMemoryManager_AddReference( &worldMesh->mem );
		return worldMesh;
	}

	NdBranch *node = ndLoadFile( path, "worldMesh" );
	if ( node == NULL )
	{
		PRINT_WARNING( "Failed to load world mesh: %s\n", path );
		return NULL;
	}

	worldMesh = YnCore_WorldMesh_Create( NULL );
	if ( YnCore_WorldDeserialiser_BeginMesh( node, worldMesh ) == NULL )
	{
		YnCore_WorldMesh_Release( worldMesh );
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
			OgeWorldFace *face = PlGetLinkedListNodeUserData( faceNode );

			unsigned int numTriangles;
			unsigned int *indices  = ogeWorld_ConvertFaceToTriangles( face, &numTriangles );
			unsigned int *curIndex = indices;
			for ( unsigned int k = 0; k < numTriangles; ++k, curIndex += 3 )
				PlgAddMeshTriangle( worldMesh->drawMesh, curIndex[ 0 ], curIndex[ 1 ], curIndex[ 2 ] );

			PL_DELETE( indices );

			oge_RendererPerformance_.numFacesDrawn++;

			faceNode = PlGetNextLinkedListNode( faceNode );
		}

		PlgGenerateVertexTangentBasis( worldMesh->drawMesh->vertices, worldMesh->drawMesh->num_verts );
		//PlgGenerateMeshTangentBasis( worldMesh->drawMesh );

		ogeMM_AddToCache( path, MEM_CACHE_WORLD_MESH, worldMesh );
	}

	return worldMesh;
}

void YnCore_WorldMesh_Release( OgeWorldMesh *worldMesh )
{
	if ( worldMesh == NULL )
		return;

	ogeMemoryManager_ReleaseReference( &worldMesh->mem );
}
