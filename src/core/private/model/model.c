// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Model deserialization and caching.
// Author:  Mark E. Sowden

#include "ape_private.h"
#include "model.h"

#include "ape/ape_formats.h"

#include "plcore/pl_hashtable.h"

#include <yin/node.h>

/////////////////////////////////////////////////////////////////////////////////////
// Private

static PLHashTable *modelsTable;

/**
 * Callback for garbage day.
 */
static void destroy_model( void *userData )
{
	SSApeModel *model = ( SSApeModel * ) userData;
	assert( model != NULL );

	for ( unsigned int i = 0; i < model->numMaterials; ++i )
		ss_arl_material_release( model->materials[ i ] );

	PlSetHashTableNodeUserData( model->node, NULL );

	PL_DELETE( model );
}

static PLGMesh *deserialize_mesh( SSApeModel *model, NdBranch *root )
{
	NdBranch *child;

	PLVector3 *positions = NULL;
	unsigned int numVertices = 0;
	if ( ( child = ndGetChildByName( root, "positions" ) ) != NULL )
	{
		numVertices = ndGetNumOfChildren( child );
		if ( numVertices >= 3 )
		{
			positions = PL_NEW_( PLVector3, numVertices );
			numVertices = numVertices / 3;
			ndGetF32Array( child, ( float * ) positions, numVertices );
		}
		else
		{
			PRINT_WARNING( "Invalid number (%u) of positions in model!\n", numVertices );
			numVertices = 0;
		}
	}
	else
		PRINT_WARNING( "Mesh has no vertices!\n" );

	PLVector3 *normals = NULL;
	unsigned int numNormals = 0;
	if ( ( child = ndGetChildByName( root, "normals" ) ) != NULL )
	{
		numNormals = ndGetNumOfChildren( child );
		if ( numNormals >= 3 )
		{
			normals = PL_NEW_( PLVector3, numNormals );
			numNormals = numNormals / 3;
			ndGetF32Array( child, ( float * ) normals, numNormals );
		}
		else
		{
			PRINT_WARNING( "Invalid number (%u) of normals in model!\n", numNormals );
			numNormals = 0;
		}
	}

	PLVector2 *uvs = NULL;
	unsigned int numUVs = 0;
	if ( ( child = ndGetChildByName( root, "uvs" ) ) != NULL )
	{
		numUVs = ndGetNumOfChildren( child );
		if ( numUVs >= 2 )
		{
			uvs = PL_NEW_( PLVector2, numUVs );
			numUVs = numUVs / 2;
			ndGetF32Array( child, ( float * ) uvs, numUVs );
		}
		else
		{
			PRINT_WARNING( "Invalid number (%u) of UVs in model!\n", numUVs );
			numUVs = 0;
		}
	}

	PLColourF32 *colours = NULL;
	unsigned int numColours = 0;
	if ( ( child = ndGetChildByName( root, "colours" ) ) != NULL )
	{
		numColours = ndGetNumOfChildren( child );
		if ( numColours >= 4 )
		{
			colours = PL_NEW_( PLColourF32, numVertices );
			numColours = numColours / 4;
			ndGetF32Array( child, ( float * ) colours, numColours );
		}
		else
		{
			PRINT_WARNING( "Invalid number (%u) of colours in model!\n", numColours );
			numColours = 0;
		}
	}

	unsigned int *indices = NULL;
	unsigned int numIndices = 0;
	unsigned int numTriangles = 0;
	if ( ( child = ndGetChildByName( root, "triangles" ) ) != NULL )
	{
		numIndices = ndGetNumOfChildren( child );
		indices = PL_NEW_( unsigned int, numIndices );
		ndGetUI32Array( child, indices, numIndices );
		numTriangles = numIndices / 3;
	}
	else
		PRINT_WARNING( "Mesh has no indices!\n" );

	unsigned int materialIndex = ndGetUInt( root, "materialIndex", 0 );
	if ( materialIndex >= SS_APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		PRINT_WARNING( "Material index (%u) exceeds material limit (%u)!\n", materialIndex, SS_APE_FORMAT_MODEL_MAX_MATERIALS );
		materialIndex = 0;
	}

	if ( model->meshes[ materialIndex ] == NULL )
		model->meshes[ materialIndex ] = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, numVertices );
	if ( model->meshes[ materialIndex ] != NULL )
	{
		for ( unsigned int i = 0; i < numVertices; ++i )
		{
			PLColour colour = ( colours == NULL ) ? ( PLColour ){ 255, 255, 255, 255 } : PlColourF32ToU8( &colours[ i ] );
			PLVector3 normal = ( normals == NULL ) ? pl_vecOrigin3 : normals[ i ];
			PLVector2 uv = ( uvs == NULL ) ? pl_vecOrigin2 : uvs[ i ];
			PlgAddMeshVertex( model->meshes[ materialIndex ], &positions[ i ], &normal, &colour, &uv );
		}

		for ( unsigned int i = 0; i < numIndices; i += 3 )
			PlgAddMeshTriangle( model->meshes[ materialIndex ], indices[ i ], indices[ i + 1 ], indices[ i + 2 ] );

		if ( normals == NULL )
			PlgGenerateMeshNormals( model->meshes[ materialIndex ], false );

		PlgGenerateMeshTangentBasis( model->meshes[ materialIndex ] );
		PlgUploadMesh( model->meshes[ materialIndex ] );
	}
	else
		PRINT_WARNING( "Failed to create mesh: %s\n", PlGetError() );

	PL_DELETE( positions );
	PL_DELETE( normals );
	PL_DELETE( uvs );
	PL_DELETE( colours );
	PL_DELETE( indices );

	return model->meshes[ materialIndex ];
}

static SSApeModel *deserialize_model( NdBranch *root )
{
	unsigned int version = ndGetUInt( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 || version > SS_APE_FORMAT_MODEL_VERSION )
	{
		PRINT_WARNING( "Invalid model version, %d, expected %u!\n", version, SS_APE_FORMAT_MODEL_VERSION );
		return NULL;
	}

	unsigned int numMeshes;
	NdBranch *meshArray = ndGetChildByName( root, "meshes" );
	if ( meshArray == NULL || ( ( numMeshes = ndGetNumOfChildren( meshArray ) ) == 0 ) )
	{
		PRINT_WARNING( "No meshes for model!\n" );
		return NULL;
	}
	else if ( numMeshes >= SS_APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		PRINT_WARNING( "Unexpected number of meshes (%u >= %u)!\n", numMeshes, SS_APE_FORMAT_MODEL_MAX_MATERIALS );
		numMeshes = ( SS_APE_FORMAT_MODEL_MAX_MATERIALS - 1 );
	}

	SSApeModel *model = PL_NEW( SSApeModel );

	// Iterate over all the materials under the root and attempt to load them all in
	NdBranch *materialArray = ndGetChildByName( root, "materials" );
	if ( materialArray == NULL )
	{
		PRINT_WARNING( "No materials for model, using fallback!\n" );
		model->numMaterials = 1;
		model->materials[ 0 ] = ss_arl_material_cache( "materials/engine/fallback_mesh.mat.n", 0, true, false );
	}
	else
	{
		model->numMaterials = ndGetNumOfChildren( materialArray );
		NdBranch *n = ndGetFirstChild( materialArray );
		for ( unsigned int i = 0; i < model->numMaterials; ++i )
		{
			assert( n != NULL );
			char materialPath[ PL_SYSTEM_MAX_PATH ];
			if ( ndGetStr( n, materialPath, sizeof( materialPath ) ) != ND_ERROR_SUCCESS )
			{
				PRINT_WARNING( "Failed to get material string for model: %s\n", ndGetErrorMessage() );
				model->materials[ i ] = ss_arl_material_cache( "materials/engine/fallback_mesh.mat.n", 0, true, false );
			}
			else
				model->materials[ i ] = ss_arl_material_cache( materialPath, 0, true, false );

			n = ndGetNextChild( n );
		}
	}

	NdBranch *meshNode = ndGetFirstChild( meshArray );
	for ( unsigned int i = 0; i < numMeshes; ++i )
	{
		assert( meshNode != NULL );
		deserialize_mesh( model, meshNode );
		meshNode = ndGetNextChild( meshNode );
	}

	NdBranch *bonesList = ndGetChildByName( root, "bones" );
	if ( bonesList != NULL )
	{
		model->numBones = ndGetNumOfChildren( bonesList );
		if ( model->numBones >= SS_APE_FORMAT_MODEL_MAX_BONES )
		{
			PRINT_WARNING( "Unexpected number of bones (%u >= %u)!", model->numBones, SS_APE_FORMAT_MODEL_MAX_BONES );
			model->numBones = ( SS_APE_FORMAT_MODEL_MAX_BONES - 1 );
		}
		NdBranch *child = ndGetFirstChild( bonesList );
		for ( unsigned int i = 0; i < model->numBones; ++i )
		{
			if ( child == NULL )
				break;

			child = ndGetNextChild( child );
		}

		unsigned int rootBone = ndGetUInt( root, "rootBone", 0 );
		if ( rootBone >= model->numBones )
		{
			PRINT_WARNING( "Invalid root bone (%u), defaulting to 0!\n", rootBone );
			rootBone = 0;
		}
		model->rootBone = &model->bones[ rootBone ];
	}

	return model;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

SSApeModel *ss_ape_model_load( const char *path )
{
	if ( modelsTable == NULL )
		modelsTable = PlCreateHashTable();

	SSApeModel *model = PlLookupHashTableUserData( modelsTable, path, strlen( path ) );
	if ( model != NULL )
		return model;

	NdBranch *root = ndLoadFile( path, "model" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Invalid model: %s (%s)\n", ndGetErrorMessage() );
		return NULL;
	}

	model = deserialize_model( root );
	if ( model == NULL )
		PRINT_WARNING( "Failed to load model, \"%s\"!\n", path );

	ndDestroyBranch( root );

	//PlInsertHashTableNode( modelsTable, path, strlen( path ), model );

	return model;
}

/**
 * Release a model handle.
 * If it's not tracked by the memory
 * manager then it'll be immediately
 * destroyed.
 */
void ss_ape_model_release( SSApeModel *model )
{
	//TODO!!!
	assert( 0 );

	ss_acl_mm_release( &model->mem );
}

void ss_ape_model_draw( SSApeModel *model )
{
}
