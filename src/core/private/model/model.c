// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Model deserialization and caching.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "ape_private.h"
#include "model.h"
#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void model_cleanup_callback_( void *userData )
{
	ApeModel *model = ( ApeModel * ) userData;
	assert( model != NULL );

	for ( uint i = 0; i < model->numMaterials; ++i )
	{
		ape_material_release( model->meshes[ i ].material );
		PlgDestroyMesh( model->meshes[ i ].cache );
	}

	PL_DELETE( model );
}

static ApeModelMesh *deserialize_mesh( ApeModel *model, ApeModelMesh *mesh, AcmBranch *root )
{
	const char *materialPath = acm_branch_get_child_string( root, "material", nullptr );
	if ( materialPath == nullptr )
	{
		materialPath = "materials/engine/fallback.mat.n";
	}

	mesh->material = ape_material_cache( materialPath, APE_CACHE_GROUP_WORLD, true, false );

	AcmBranch *branch;

	PLVector3 *positions   = NULL;
	uint       numVertices = 0;
	if ( ( branch = acm_branch_get_child_by_name( root, "positions" ) ) != NULL )
	{
		numVertices = acm_branch_get_num_of_children( branch );
		if ( numVertices >= 3 )
		{
			positions   = PL_NEW_( PLVector3, numVertices );
			numVertices = numVertices / 3;
			acm_branch_get_float32_array( branch, ( float * ) positions, numVertices );
		}
		else
		{
			ape_warning_( "Invalid number (%u) of positions in model!\n", numVertices );
			numVertices = 0;
		}
	}

	if ( numVertices == 0 )
	{
		ape_warning_( "Mesh has no vertices!\n" );
		return nullptr;
	}

	PLVector3 *normals    = NULL;
	uint       numNormals = 0;
	if ( ( branch = acm_branch_get_child_by_name( root, "normals" ) ) != NULL )
	{
		numNormals = acm_branch_get_num_of_children( branch );
		if ( numNormals >= 3 )
		{
			normals    = PL_NEW_( PLVector3, numNormals );
			numNormals = numNormals / 3;
			acm_branch_get_float32_array( branch, ( float * ) normals, numNormals );
		}
		else
		{
			ape_warning_( "Invalid number (%u) of normals in model!\n", numNormals );
			numNormals = 0;
		}
	}

	PLVector2 *uvs    = NULL;
	uint       numUVs = 0;
	if ( ( branch = acm_branch_get_child_by_name( root, "uvs" ) ) != NULL )
	{
		numUVs = acm_branch_get_num_of_children( branch );
		if ( numUVs >= 2 )
		{
			uvs    = PL_NEW_( PLVector2, numUVs );
			numUVs = numUVs / 2;
			acm_branch_get_float32_array( branch, ( float * ) uvs, numUVs );
		}
		else
		{
			ape_warning_( "Invalid number (%u) of UVs in model!\n", numUVs );
			numUVs = 0;
		}
	}

	PLColourF32 *colours    = NULL;
	uint         numColours = 0;
	if ( ( branch = acm_branch_get_child_by_name( root, "colours" ) ) != NULL )
	{
		numColours = acm_branch_get_num_of_children( branch );
		if ( numColours >= 4 )
		{
			colours    = PL_NEW_( PLColourF32, numVertices );
			numColours = numColours / 4;
			acm_branch_get_float32_array( branch, ( float * ) colours, numColours );
		}
		else
		{
			ape_warning_( "Invalid number (%u) of colours in model!\n", numColours );
			numColours = 0;
		}
	}

	uint *indices      = NULL;
	uint  numIndices   = 0;
	uint  numTriangles = 0;
	if ( ( branch = acm_branch_get_child_by_name( root, "triangles" ) ) != NULL )
	{
		numIndices = acm_branch_get_num_of_children( branch );
		indices    = PL_NEW_( uint, numIndices );
		acm_branch_get_uint32_array( branch, indices, numIndices );
		numTriangles = numIndices / 3;
	}
	else
	{
		ape_warning_( "Mesh has no indices!\n" );
	}

	uint materialIndex = acm_branch_get_child_uint( root, "materialIndex", 0 );
	if ( materialIndex >= APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		ape_warning_( "Material index (%u) exceeds material limit (%u)!\n", materialIndex, APE_FORMAT_MODEL_MAX_MATERIALS );
		materialIndex = 0;
	}

	if ( model->meshes[ materialIndex ].cache == NULL )
	{
		model->meshes[ materialIndex ].cache = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, numVertices );
	}
	if ( model->meshes[ materialIndex ].cache != NULL )
	{
		for ( uint i = 0; i < numVertices; ++i )
		{
			PLColour  colour = ( colours == NULL ) ? ( PLColour ){ 255, 255, 255, 255 } : PlColourF32ToU8( &colours[ i ] );
			PLVector3 normal = ( normals == NULL ) ? pl_vecOrigin3 : normals[ i ];
			PLVector2 uv     = ( uvs == NULL ) ? pl_vecOrigin2 : uvs[ i ];
			PlgAddMeshVertex( model->meshes[ materialIndex ].cache, &positions[ i ], &normal, &colour, &uv );
		}

		for ( uint i = 0; i < numIndices; i += 3 )
		{
			PlgAddMeshTriangle( model->meshes[ materialIndex ].cache, indices[ i ], indices[ i + 1 ], indices[ i + 2 ] );
		}

		if ( normals == NULL )
		{
			PlgGenerateMeshNormals( model->meshes[ materialIndex ].cache, false );
		}

		PlgGenerateMeshTangentBasis( model->meshes[ materialIndex ].cache );
		PlgUploadMesh( model->meshes[ materialIndex ].cache );
	}
	else
	{
		ape_warning_( "Failed to create mesh: %s\n", PlGetError() );
	}

	PL_DELETE( positions );
	PL_DELETE( normals );
	PL_DELETE( uvs );
	PL_DELETE( colours );
	PL_DELETE( indices );

	return mesh;
}

static ApeModel *deserialize_model( ApeModel *model, AcmBranch *root )
{
	uint version = acm_branch_get_child_uint( root, "version", ( uint ) -1 );
	if ( version == ( uint ) -1 || version > APE_FORMAT_MODEL_VERSION )
	{
		ape_warning_( "Invalid model version, %d, expected %u!\n", version, APE_FORMAT_MODEL_VERSION );
		return NULL;
	}

	uint       numMeshes;
	AcmBranch *meshArray = acm_branch_get_child_by_name( root, "meshes" );
	if ( meshArray == NULL || ( ( numMeshes = acm_branch_get_num_of_children( meshArray ) ) == 0 ) )
	{
		ape_warning_( "No meshes for model!\n" );
		return NULL;
	}
	else if ( numMeshes >= APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		ape_warning_( "Unexpected number of meshes (%u >= %u)!\n", numMeshes, APE_FORMAT_MODEL_MAX_MATERIALS );
		numMeshes = ( APE_FORMAT_MODEL_MAX_MATERIALS - 1 );
	}

	AcmBranch *meshNode = acm_branch_get_first_child( meshArray );
	for ( uint i = 0; i < numMeshes; ++i )
	{
		assert( meshNode != NULL );
		if ( deserialize_mesh( model, &model->meshes[ i ], meshNode ) == nullptr )
		{
			ape_warning_( "Failed to deserialize mesh %u!\n", i );
			break;
		}

		meshNode = acm_get_next_child( meshNode );
	}

	AcmBranch *bonesList = acm_branch_get_child_by_name( root, "bones" );
	if ( bonesList != NULL )
	{
		model->numBones = acm_branch_get_num_of_children( bonesList );
		if ( model->numBones >= APE_FORMAT_MODEL_MAX_BONES )
		{
			ape_warning_( "Unexpected number of bones (%u >= %u)!", model->numBones, APE_FORMAT_MODEL_MAX_BONES );
			model->numBones = ( APE_FORMAT_MODEL_MAX_BONES - 1 );
		}
		AcmBranch *child = acm_branch_get_first_child( bonesList );
		for ( uint i = 0; i < model->numBones; ++i )
		{
			if ( child == NULL )
			{
				break;
			}

			child = acm_get_next_child( child );
		}

		uint rootBone = acm_branch_get_child_uint( root, "rootBone", 0 );
		if ( rootBone >= model->numBones )
		{
			ape_warning_( "Invalid root bone (%u), defaulting to 0!\n", rootBone );
			rootBone = 0;
		}
		model->rootBone = &model->bones[ rootBone ];
	}

	return model;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeModel *ape_model_load( const char *path )
{
	ApeModel *model = ape_cache_get_data_( path, APE_CACHE_POOL_MODELS );
	if ( model != NULL )
	{
		ape_mm_add_reference( &model->reference );
		return model;
	}

	AcmBranch *root = acm_load_file( path, "model" );
	if ( root == NULL )
	{
		ape_warning_( "Invalid model: %s (%s)\n", acm_get_error_message(), path );
		return NULL;
	}

	model = PL_NEW( ApeModel );
	if ( deserialize_model( model, root ) != nullptr )
	{
		ape_cache_add_to_pool_( path, APE_CACHE_POOL_MODELS, model );

		ape_mm_setup_reference( "model", APE_CACHE_POOL_MODELS, &model->reference, model_cleanup_callback_, model );
		ape_mm_add_reference( &model->reference );
	}
	else
	{
		PL_DELETEN( model );
		ape_warning_( "Failed to load model, \"%s\"!\n", path );
	}

	acm_branch_destroy( root );

	return model;
}

void ape_model_release( ApeModel *model )
{
	ape_mm_release( &model->reference );
}

void ape_model_draw( ApeModel *model, const ApeModelAnimationState *state, const PLMatrix4 *transform )
{
	for ( uint i = 0; model->numMaterials; ++i )
	{
		ape_material_draw( model->meshes[ i ].material, model->meshes[ i ].cache, nullptr, 0 );
	}
}

void ape_model_draw_instanced( ApeModel *model, const PLMatrix4 **transforms, uint numTransforms )
{
	//TODO: only will work with static models for now...
}
