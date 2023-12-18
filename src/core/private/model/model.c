// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Model deserialisation and caching.
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

static PLGMesh *deserialize_mesh( NdBranch *root )
{
	NdBranch *child;

	// vertex positions are required
	PLVector3 *positions;
	unsigned int numVertices = 0;
	if ( ( child = ndGetChildByName( root, "positions" ) ) != NULL )
	{
		numVertices = ndGetNumOfChildren( child );
		positions = PL_NEW_( PLVector3, numVertices );
		ndGetF32Array( child, ( float * ) positions, numVertices / 3 );
	}





	PL_DELETE( positions );

	////////////////////////////////

	unsigned int numFaces = ndGetUInt( root, "numFaces", 0 );
	if ( numFaces > 0 )
	{
	}
	else// mesh is composed of explicit triangles
	{


		child = ndGetChildByName( root, "triangles" );
		if ( child == NULL )
		{
			PRINT_WARNING( "Invalid mesh, no triangles!\n" );
			return NULL;
		}



		if ( numVertices == 0 )
		{
			PRINT_WARNING( "Invalid mesh, no vertex positions!\n" );
			return NULL;
		}

		uint32_t numIndices = ndGetNumOfChildren( child );
		uint32_t *indices = PL_NEW_( uint32_t, numIndices );
		ndGetI32Array( child, ( int32_t * ) indices, numIndices );
		uint32_t numTriangles = numIndices / 3;

		// the rest are optional

		PLVector3 *normals = NULL;
		if ( ( child = ndGetChildByName( root, "normals" ) ) != NULL )
		{
			normals = PL_NEW_( PLVector3, numVertices );
			ndGetF32Array( child, ( float * ) normals, numVertices / 3 );
		}

		PLVector2 *uvs = NULL;
		if ( ( child = ndGetChildByName( root, "uvs" ) ) != NULL )
		{
			uvs = PL_NEW_( PLVector2, numVertices );
			ndGetF32Array( child, ( float * ) uvs, numVertices / 2 );
		}

		PLColourF32 *colours = NULL;
		if ( ( child = ndGetChildByName( root, "colours" ) ) != NULL )
		{
			colours = PL_NEW_( PLColourF32, numVertices );
			ndGetF32Array( child, ( float * ) colours, numVertices / 4 );
		}

		mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, numVertices );
		if ( mesh == NULL )
		{
			PRINT_WARNING( "Failed to create mesh: %s\n", PlGetError() );
			return NULL;
		}

		mesh->materialIndex = ndGetUInt( root, "materialIndex", 0 );

		for ( uint32_t i = 0; i < numVertices; ++i )
		{
			PLColour colour = ( colours == NULL ) ? ( PLColour ){ 255, 255, 255, 255 } : PlColourF32ToU8( &colours[ i ] );
			PLVector3 normal = ( normals == NULL ) ? pl_vecOrigin3 : normals[ i ];
			PLVector2 uv = ( uvs == NULL ) ? pl_vecOrigin2 : uvs[ i ];
			PlgAddMeshVertex( mesh, &positions[ i ], &normal, &colour, &uv );
		}

		for ( uint32_t i = 0; i < numIndices; i += 3 )
			PlgAddMeshTriangle( mesh, indices[ i ], indices[ i + 1 ], indices[ i + 2 ] );

		if ( normals == NULL )
			PlgGenerateMeshNormals( mesh, false );
	}

	PlgGenerateMeshTangentBasis( mesh );
	PlgUploadMesh( mesh );

	return mesh;
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
	else if ( numMeshes >= SS_APE_MODEL_MAX_MATERIALS )
	{
		PRINT_WARNING( "Unexpected number of meshes (%u >= %u)!\n", numMeshes, SS_APE_MODEL_MAX_MATERIALS );
		numMeshes = ( SS_APE_MODEL_MAX_MATERIALS - 1 );
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
				model->materials[ i ] = ss_arl_material_cache( "materials/engine/fallback_mesh.mat.n", 0, false, false );
				if ( model->materials[ i ] == NULL )
					PRINT_ERROR( "Failed to cache fallback material for mesh!\n" );
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
		model->meshes[ i ] = deserialize_mesh( meshNode );
		if ( model->meshes[ i ] == NULL )
			PRINT_ERROR( "Failed to load mesh %u from model!\n" );

		meshNode = ndGetNextChild( meshNode );
	}

	NdBranch *bonesList = ndGetChildByName( root, "bones" );
	if ( bonesList != NULL )
	{
		model->numBones = ndGetNumOfChildren( bonesList );
		if ( model->numBones >= SS_APE_MODEL_MAX_BONES )
		{
			PRINT_WARNING( "Unexpected number of bones (%u >= %u)!", model->numBones, SS_APE_MODEL_MAX_BONES );
			model->numBones = ( SS_APE_MODEL_MAX_BONES - 1 );
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

	PlInsertHashTableNode( modelsTable, path, strlen( path ) );

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
