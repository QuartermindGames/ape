/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "model.h"

#include <common/node.h>

/**
 * Callback for garbage day.
 */
static void MDL_CB_Destroy( void *userData )
{
	PLMModel *model = userData;
	u_assert( model != NULL );

	MDLUserData *additionalData = model->userData;
	if ( additionalData != NULL )
	{
		for ( unsigned int i = 0; i < additionalData->numMaterials; ++i )
			RM_ReleaseMaterial( additionalData->materials[ i ] );
	}

	PlmDestroyModel( model );
}

/**
 * Setup the additional data we need.
 */
static void MDL_SetupUserData( PLMModel *model )
{
	MDLUserData *userData  = globalSystem.MAlloc( sizeof( MDLUserData ), true );
	userData->numMaterials = model->numMaterials;
	if ( userData->numMaterials > MODEL_MAX_MATERIALS )
	{
		PrintWarn( "Invalid number of materials: " COM_FMT_uint32 " vs " COM_FMT_uint32 "\n",
				   userData->numMaterials, MODEL_MAX_MATERIALS );
		userData->numMaterials = MODEL_MAX_MATERIALS;
	}

	for ( unsigned int i = 0; i < userData->numMaterials; ++i )
		userData->materials[ i ] = RM_CacheMaterial( model->materials[ i ], CACHE_GROUP_WORLD, true );

	MEM_SetupReferenceInstance( "model", &userData->mem, MDL_CB_Destroy, model );
}

PLMModel *MDL_DeserializeModel( NLNode *root )
{
	int version = NL_GetI32ByName( root, "version", -1 );
	if ( version == -1 || version > 1 )
	{
		PrintWarn( "Invalid model version, %d, expected 1!\n", version );
		return NULL;
	}

	unsigned int numMeshes;
	NLNode	   *meshArray = NL_GetChildByName( root, "meshes" );
	if ( meshArray == NULL || ( ( numMeshes = NL_GetNumOfChildren( meshArray ) ) == 0 ) )
	{
		PrintWarn( "No meshes for model!\n" );
		return NULL;
	}

	MDLUserData userData;
	memset( &userData, 0, sizeof( MDLUserData ) );

	NLNode *materialArray = NL_GetChildByName( root, "materials" );
	if ( materialArray == NULL )
	{
		PrintWarn( "No materials for model, using fallback!\n" );
		userData.numMaterials	= 1;
		userData.materials[ 0 ] = RM_CacheMaterial( "materials/engine/fallback_mesh.node", 0, true );
	}
	else
	{
		userData.numMaterials = NL_GetNumOfChildren( materialArray );
		NLNode *n			  = NL_GetFirstChild( materialArray );
		for ( unsigned int i = 0; i < userData.numMaterials; ++i )
		{
			u_assert( n != NULL );

			char materialPath[ PL_SYSTEM_MAX_PATH ];
			if ( NL_GetStr( n, materialPath, sizeof( materialPath ) ) != NL_ERROR_SUCCESS )
			{
				userData.materials[ i ] = RM_CacheMaterial( "materials/engine/fallback_mesh.node", 0, false );
				if ( userData.materials[ i ] == NULL )
					PrintError( "Failed to cache fallback material for mesh!\n" );
			}
			else
				userData.materials[ i ] = RM_CacheMaterial( materialPath, 0, true );

			n = NL_GetNextChild( n );
		}
	}

	PLGMesh **meshes   = globalSystem.MAlloc( sizeof( *meshes ) * numMeshes, true );
	NLNode   *meshNode = NL_GetFirstChild( meshArray );
	for ( unsigned int i = 0; i < numMeshes; ++i )
	{
		u_assert( meshNode != NULL );

		NLNode *vertexArray = NL_GetChildByName( meshNode, "vertices" );
		if ( vertexArray == NULL )
			PrintError( "No vertices for mesh\n" );

		NLNode *faceArray = NL_GetChildByName( meshNode, "faces" );
		if ( faceArray == NULL )
			PrintError( "No faces for mesh\n" );

		unsigned int numVertices  = NL_GetNumOfChildren( vertexArray );
		unsigned int numFaces	  = NL_GetNumOfChildren( faceArray );
		unsigned int numTriangles = numFaces / 3;

		meshes[ i ] = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_DYNAMIC, numTriangles, numVertices );
		if ( meshes[ i ] == NULL )
			PrintError( "Failed to create mesh %d\n", i );

		meshes[ i ]->materialIndex = NL_GetI32ByName( meshNode, "materialIndex", 0 );

		NLNode *vertexNode = NL_GetFirstChild( vertexArray );
		for ( unsigned int j = 0; j < numVertices; ++j )
		{
			NLNode *n;

			PLVector3 position = pl_vecOrigin3;
			if ( ( n = NL_GetChildByName( vertexNode, "position" ) ) != NULL )
			{
				NL_DS_DeserializeVector3( n, &position );
				//Print( "p %d : %s\n", j, PlPrintVector3( &position, pl_float_var ) );
			}

			PLVector3 normal = pl_vecOrigin3;
			if ( ( n = NL_GetChildByName( vertexNode, "normal" ) ) != NULL )
				NL_DS_DeserializeVector3( n, &normal );

			PLColour colour = PLColourRGB( 255, 255, 255 );
			if ( ( n = NL_GetChildByName( vertexNode, "colour" ) ) != NULL )
				NL_DS_DeserializeColour( n, &colour );

			PLVector2 uv = pl_vecOrigin2;
			if ( ( n = NL_GetChildByName( vertexNode, "textureCoords" ) ) != NULL )
				NL_DS_DeserializeVector2( n, &uv );

			PlgAddMeshVertex( meshes[ i ], position, normal, colour, uv );

			vertexNode = NL_GetNextChild( vertexNode );
		}

		NLNode *faceNode = NL_GetFirstChild( faceArray );
		for ( unsigned int j = 0; j < numFaces; ++j )
		{
			NLNode *n;
			if ( ( n = NL_GetChildByName( faceNode, "indices" ) ) != NULL )
			{
				int indices[ 3 ];
				if ( NL_GetI32Array( n, indices, 3 ) == NL_ERROR_SUCCESS )
				{
					//Print( "xi %d yi %d zi %d\n", indices[ 0 ], indices[ 1 ], indices[ 2 ] );
					PlgAddMeshTriangle( meshes[ i ], indices[ 0 ], indices[ 1 ], indices[ 2 ] );
				}
				else
					PrintWarn( "Failed to fetch indices for face " COM_FMT_int32 "\n", j );
			}

			faceNode = NL_GetNextChild( faceNode );
		}

		PlgGenerateTangentBasis( meshes[ i ]->vertices, meshes[ i ]->num_verts, meshes[ i ]->indices, meshes[ i ]->num_triangles );
		PlgUploadMesh( meshes[ i ] );

		meshNode = NL_GetNextChild( meshNode );
	}

	PLMModel *model = PlmCreateStaticModel( meshes, numMeshes );
	PlmGenerateModelBounds( model );

	model->userData							   = globalSystem.MAlloc( sizeof( MDLUserData ), true );
	*( ( MDLUserData * ) ( model->userData ) ) = userData;

	return model;
}

PLMModel *MDL_CacheModel( const char *path )
{
	NLNode *root = NL_LoadFile( path, "model" );
	if ( root == NULL )
	{
		PrintWarn( "Invalid model: %s (%s)\n", NL_GetErrorMessage() );
		return NULL;
	}

	PLMModel *model = MDL_DeserializeModel( root );
	if ( model == NULL )
		PrintWarn( "Failed to load model, \"%s\"!\n", path );

	NL_DestroyNode( root );

	return model;
}

/**
 * Release a model handle.
 * If it's not tracked by the memory
 * manager then it'll be immediately
 * destroyed.
 */
void MDL_ReleaseModel( PLMModel *model )
{
	MDLUserData *additionalData = model->userData;
	if ( additionalData == NULL )
	{
		PrintWarn( "Destroying model not tracked by memory manager!\n" );
		PlmDestroyModel( model );
		return;
	}

	MEM_ReleaseReference( &additionalData->mem );
}
