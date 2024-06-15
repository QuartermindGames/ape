// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
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
	ApeModel *model = ( ApeModel * ) userData;
	assert( model != NULL );

	for ( unsigned int i = 0; i < model->numMaterials; ++i )
		ape_material_release( model->materials[ i ] );

	PlSetHashTableNodeUserData( model->node, NULL );

	PL_DELETE( model );
}

static PLGMesh *deserialize_mesh( ApeModel *model, NdBranch *root )
{
	NdBranch *child;

	PLVector3 *positions = NULL;
	unsigned int numVertices = 0;
	if ( ( child = nd_branch_get_child_by_name( root, "positions" ) ) != NULL )
	{
		numVertices = nd_branch_get_num_of_children( child );
		if ( numVertices >= 3 )
		{
			positions = PL_NEW_( PLVector3, numVertices );
			numVertices = numVertices / 3;
			nd_branch_get_float32_array( child, ( float * ) positions, numVertices );
		}
		else
		{
			PRINT_WARNING( "Invalid number (%u) of positions in model!\n", numVertices );
			numVertices = 0;
		}
	}
	else
	{
		PRINT_WARNING( "Mesh has no vertices!\n" );
	}

	PLVector3 *normals = NULL;
	unsigned int numNormals = 0;
	if ( ( child = nd_branch_get_child_by_name( root, "normals" ) ) != NULL )
	{
		numNormals = nd_branch_get_num_of_children( child );
		if ( numNormals >= 3 )
		{
			normals = PL_NEW_( PLVector3, numNormals );
			numNormals = numNormals / 3;
			nd_branch_get_float32_array( child, ( float * ) normals, numNormals );
		}
		else
		{
			PRINT_WARNING( "Invalid number (%u) of normals in model!\n", numNormals );
			numNormals = 0;
		}
	}

	PLVector2 *uvs = NULL;
	unsigned int numUVs = 0;
	if ( ( child = nd_branch_get_child_by_name( root, "uvs" ) ) != NULL )
	{
		numUVs = nd_branch_get_num_of_children( child );
		if ( numUVs >= 2 )
		{
			uvs = PL_NEW_( PLVector2, numUVs );
			numUVs = numUVs / 2;
			nd_branch_get_float32_array( child, ( float * ) uvs, numUVs );
		}
		else
		{
			PRINT_WARNING( "Invalid number (%u) of UVs in model!\n", numUVs );
			numUVs = 0;
		}
	}

	PLColourF32 *colours = NULL;
	unsigned int numColours = 0;
	if ( ( child = nd_branch_get_child_by_name( root, "colours" ) ) != NULL )
	{
		numColours = nd_branch_get_num_of_children( child );
		if ( numColours >= 4 )
		{
			colours = PL_NEW_( PLColourF32, numVertices );
			numColours = numColours / 4;
			nd_branch_get_float32_array( child, ( float * ) colours, numColours );
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
	if ( ( child = nd_branch_get_child_by_name( root, "triangles" ) ) != NULL )
	{
		numIndices = nd_branch_get_num_of_children( child );
		indices = PL_NEW_( unsigned int, numIndices );
		nd_branch_get_uint32_array( child, indices, numIndices );
		numTriangles = numIndices / 3;
	}
	else
		PRINT_WARNING( "Mesh has no indices!\n" );

	unsigned int materialIndex = nd_branch_get_child_uint( root, "materialIndex", 0 );
	if ( materialIndex >= APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		PRINT_WARNING( "Material index (%u) exceeds material limit (%u)!\n", materialIndex, APE_FORMAT_MODEL_MAX_MATERIALS );
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

static ApeModel *deserialize_model( NdBranch *root )
{
	unsigned int version = nd_branch_get_child_uint( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 || version > APE_FORMAT_MODEL_VERSION )
	{
		PRINT_WARNING( "Invalid model version, %d, expected %u!\n", version, APE_FORMAT_MODEL_VERSION );
		return NULL;
	}

	unsigned int numMeshes;
	NdBranch *meshArray = nd_branch_get_child_by_name( root, "meshes" );
	if ( meshArray == NULL || ( ( numMeshes = nd_branch_get_num_of_children( meshArray ) ) == 0 ) )
	{
		PRINT_WARNING( "No meshes for model!\n" );
		return NULL;
	}
	else if ( numMeshes >= APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		PRINT_WARNING( "Unexpected number of meshes (%u >= %u)!\n", numMeshes, APE_FORMAT_MODEL_MAX_MATERIALS );
		numMeshes = ( APE_FORMAT_MODEL_MAX_MATERIALS - 1 );
	}

	ApeModel *model = PL_NEW( ApeModel );

	// Iterate over all the materials under the root and attempt to load them all in
	NdBranch *materialArray = nd_branch_get_child_by_name( root, "materials" );
	if ( materialArray == NULL )
	{
		PRINT_WARNING( "No materials for model, using fallback!\n" );
		model->numMaterials = 1;
		model->materials[ 0 ] = ape_material_cache( "materials/engine/fallback_mesh.mat.n", 0, true, false );
	}
	else
	{
		model->numMaterials = nd_branch_get_num_of_children( materialArray );
		NdBranch *n = nd_branch_get_first_child( materialArray );
		for ( unsigned int i = 0; i < model->numMaterials; ++i )
		{
			assert( n != NULL );
			char materialPath[ PL_SYSTEM_MAX_PATH ];
			if ( nd_branch_get_string( n, materialPath, sizeof( materialPath ) ) != ND_ERROR_SUCCESS )
			{
				PRINT_WARNING( "Failed to get material string for model: %s\n", nd_get_error_message() );
				model->materials[ i ] = ape_material_cache( "materials/engine/fallback_mesh.mat.n", 0, true, false );
			}
			else
				model->materials[ i ] = ape_material_cache( materialPath, 0, true, false );

			n = nd_get_next_child( n );
		}
	}

	NdBranch *meshNode = nd_branch_get_first_child( meshArray );
	for ( unsigned int i = 0; i < numMeshes; ++i )
	{
		assert( meshNode != NULL );
		deserialize_mesh( model, meshNode );
		meshNode = nd_get_next_child( meshNode );
	}

	NdBranch *bonesList = nd_branch_get_child_by_name( root, "bones" );
	if ( bonesList != NULL )
	{
		model->numBones = nd_branch_get_num_of_children( bonesList );
		if ( model->numBones >= APE_FORMAT_MODEL_MAX_BONES )
		{
			PRINT_WARNING( "Unexpected number of bones (%u >= %u)!", model->numBones, APE_FORMAT_MODEL_MAX_BONES );
			model->numBones = ( APE_FORMAT_MODEL_MAX_BONES - 1 );
		}
		NdBranch *child = nd_branch_get_first_child( bonesList );
		for ( unsigned int i = 0; i < model->numBones; ++i )
		{
			if ( child == NULL )
				break;

			child = nd_get_next_child( child );
		}

		unsigned int rootBone = nd_branch_get_child_uint( root, "rootBone", 0 );
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

ApeModel *ape_load_model( const char *path )
{
	if ( modelsTable == NULL )
		modelsTable = PlCreateHashTable();

	ApeModel *model = PlLookupHashTableUserData( modelsTable, path, strlen( path ) );
	if ( model != NULL )
		return model;

	NdBranch *root = nd_load_file( path, "model" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Invalid model: %s (%s)\n", nd_get_error_message() );
		return NULL;
	}

	model = deserialize_model( root );
	if ( model == NULL )
		PRINT_WARNING( "Failed to load model, \"%s\"!\n", path );

	nd_branch_destroy( root );

	//PlInsertHashTableNode( modelsTable, path, strlen( path ), model );

	return model;
}

/**
 * Release a model handle.
 * If it's not tracked by the memory
 * manager then it'll be immediately
 * destroyed.
 */
void ape_model_release( ApeModel *model )
{
	//TODO!!!
	assert( 0 );

	ape_mm_release( &model->mem );
}

void ape_model_draw( ApeModel *model )
{
}
