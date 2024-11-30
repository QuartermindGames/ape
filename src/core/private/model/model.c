// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Model deserialization and caching.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "ape_private.h"
#include "model.h"
#include "world/world.h"
#include "renderer/renderer.h"

static void model_cleanup_callback_( void *userData )
{
	ApeModel *model = ( ApeModel * ) userData;
	assert( model != NULL );

	for ( uint i = 0; i < model->numMaterials; ++i )
	{
		ape_material_release( model->meshes[ i ].material );
	}

	PlgDestroyMesh( model->cache );

	PL_DELETE( model );
}

static ApeModelMesh *deserialize_mesh( ApeModel *model, ApeModelMesh *mesh, AcmBranch *root )
{
	const char *materialPath = acm_get_string( root, "material", nullptr );
	if ( materialPath == nullptr )
	{
		ape_warning_( "No material provided for mesh!\n" );
		return nullptr;
	}

	mesh->material = ape_material_cache( materialPath, APE_CACHE_GROUP_WORLD, true );

	AcmBranch *branch;
	if ( ( branch = acm_get_child_by_name( root, "triangles" ) ) != NULL )
	{
		uint i = 0;
		branch = acm_get_first_child( branch );
		while ( branch != nullptr )
		{
			uint vertexIndices[ 3 ];

			AcmBranch *childBranch;
			if ( ( childBranch = acm_get_child_by_name( branch, "vertex" ) ) != nullptr )
			{
				acm_branch_get_uint32_array( childBranch, vertexIndices, 3 );
			}

			uint tri = PlgAddMeshTriangle( model->cache, vertexIndices[ 0 ], vertexIndices[ 1 ], vertexIndices[ 2 ] );
			if ( i == 0 )
			{
				mesh->startIndex = tri;
			}

			i = tri;

			branch = acm_get_next_child( branch );
		}

		mesh->endIndex = i;
	}
	else
	{
		ape_warning_( "Mesh has no indices!\n" );
	}

	return mesh;
}

static ApeModel *deserialize_model( ApeModel *model, AcmBranch *root )
{
	uint version = acm_get_uint( root, "version", ( uint ) -1 );
	if ( version == ( uint ) -1 || version > APE_FORMAT_MODEL_VERSION )
	{
		ape_warning_( "Invalid model version, %d, expected %u!\n", version, APE_FORMAT_MODEL_VERSION );
		return nullptr;
	}

	AcmBranch *branch;

	uint numFloatElements;
	if ( ( branch = acm_get_child_by_name( root, "vertexFormatDescriptor" ) ) != nullptr )
	{
		numFloatElements = acm_get_uint( branch, "numFloatElements", 0 );
		if ( numFloatElements == 0 )
		{
			ape_warning_( "Invalid number of float elements per vertex descriptor!\n" );
			return nullptr;
		}
	}
	else
	{
		ape_warning_( "Mesh has no vertex descriptor!\n" );
		return nullptr;
	}

	float *vertices    = nullptr;
	uint   numVertices = 0;
	if ( ( branch = acm_get_child_by_name( root, "vertices" ) ) != nullptr )
	{
		uint numIndices = acm_get_num_of_children( branch );
		if ( numIndices >= 3 )
		{
			vertices    = PL_NEW_( float, numIndices );
			numVertices = numIndices / numFloatElements;
			acm_branch_get_float32_array( branch, vertices, numIndices );
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

	model->cache = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, 0, numVertices );
	if ( model->cache == nullptr )
	{
		ape_warning_( "Failed to create cache for model: %s\n", PlGetError() );
		return nullptr;
	}

	const float *v = vertices;
	for ( uint i = 0; i < numVertices; ++i, v += numFloatElements )
	{
		PlgAddMeshVertex( model->cache,
		                  ( const PLVector3 * ) &v[ 0 ],
		                  ( const PLVector3 * ) &v[ 3 ], &PL_COLOUR_WHITE,
		                  ( const PLVector2 * ) &v[ 6 ] );
	}

	AcmBranch *meshArray = acm_get_child_by_name( root, "meshes" );
	if ( meshArray == NULL || ( ( model->numMaterials = acm_get_num_of_children( meshArray ) ) == 0 ) )
	{
		ape_warning_( "No meshes for model!\n" );
		return nullptr;
	}
	else if ( model->numMaterials >= APE_FORMAT_MODEL_MAX_MATERIALS )
	{
		ape_warning_( "Unexpected number of meshes (%u >= %u)!\n", model->numMaterials, APE_FORMAT_MODEL_MAX_MATERIALS );
		model->numMaterials = ( APE_FORMAT_MODEL_MAX_MATERIALS - 1 );
	}

	if ( meshArray != nullptr )
	{
		AcmBranch *meshNode = acm_get_first_child( meshArray );
		for ( uint i = 0; i < model->numMaterials; ++i )
		{
			assert( meshNode != NULL );
			if ( deserialize_mesh( model, &model->meshes[ i ], meshNode ) == nullptr )
			{
				ape_warning_( "Failed to deserialize mesh %u!\n", i );
				break;
			}

			meshNode = acm_get_next_child( meshNode );
		}
	}

	AcmBranch *bonesList = acm_get_child_by_name( root, "bones" );
	if ( bonesList != NULL )
	{
		model->numBones = acm_get_num_of_children( bonesList );
		if ( model->numBones >= APE_FORMAT_MODEL_MAX_BONES )
		{
			ape_warning_( "Unexpected number of bones (%u >= %u)!", model->numBones, APE_FORMAT_MODEL_MAX_BONES );
			model->numBones = ( APE_FORMAT_MODEL_MAX_BONES - 1 );
		}
		AcmBranch *child = acm_get_first_child( bonesList );
		for ( uint i = 0; i < model->numBones; ++i )
		{
			if ( child == NULL )
			{
				break;
			}

			child = acm_get_next_child( child );
		}

		uint rootBone = acm_get_uint( root, "rootBone", 0 );
		if ( rootBone >= model->numBones )
		{
			ape_warning_( "Invalid root bone (%u), defaulting to 0!\n", rootBone );
			rootBone = 0;
		}
		model->rootBone = &model->bones[ rootBone ];
	}

	PL_DELETE( vertices );

	return model;
}

ApeModel *ape_model_load( const char *path )
{
	ApeModel *model = ape_memory_get_from_pool_( path, APE_CACHE_POOL_MODELS );
	if ( model != NULL )
	{
		ape_memory_add_reference( &model->reference );
		return model;
	}

	AcmBranch *root = acm_load_file( path, "model" );
	if ( root == NULL )
	{
		ape_warning_( "Invalid model: %s (%s)\n", acm_get_error_message(), path );
		return nullptr;
	}

	model = PL_NEW( ApeModel );
	if ( deserialize_model( model, root ) != nullptr )
	{
		ape_memory_setup_reference( path, APE_CACHE_POOL_MODELS, &model->reference, model_cleanup_callback_, model );
		ape_memory_add_reference( &model->reference );
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
	ape_memory_release( &model->reference );
}

void ape_model_draw( ApeModel *model, const ApeModelAnimationState *state, const PLMatrix4 *transform, ApeLight *light )
{
	for ( uint i = 0; i < model->numMaterials; ++i )
	{
		model->cache->startIndex = model->meshes[ i ].startIndex;
		model->cache->endIndex   = model->meshes[ i ].endIndex;

		ApeLightPointerArray lights;
		lights[ 0 ] = light;

		ape_material_draw( model->meshes[ i ].material, model->cache, lights );
	}
}

void ape_model_draw_instanced( ApeModel *model, const PLMatrix4 **transforms, uint numTransforms )
{
	//TODO: only will work with static models for now...
}

static PLCollisionAABB compute_model_bounds( ApeModel *model )
{
	PLCollisionAABB bounds = {};
	assert( model->cache->num_verts > 0 );
	float max = model->cache->vertices[ 0 ].position.x;
	float min = model->cache->vertices[ 0 ].position.x;
	for ( uint i = 0; i < model->cache->num_verts; ++i )
	{
		if ( model->cache->vertices[ i ].position.x > max ) max = model->cache->vertices[ i ].position.x;
		if ( model->cache->vertices[ i ].position.y > max ) max = model->cache->vertices[ i ].position.y;
		if ( model->cache->vertices[ i ].position.z > max ) max = model->cache->vertices[ i ].position.z;
		if ( model->cache->vertices[ i ].position.x < min ) min = model->cache->vertices[ i ].position.x;
		if ( model->cache->vertices[ i ].position.y < min ) min = model->cache->vertices[ i ].position.y;
		if ( model->cache->vertices[ i ].position.z < min ) min = model->cache->vertices[ i ].position.z;
	}

	if ( ( min * -1 ) > max ) max = ( min * -1 );
	bounds.mins = PL_VECTOR3( -max, -max, -max );
	bounds.maxs = PL_VECTOR3( max, max, max );

	return bounds;
}

/////////////////////////////////////////////////////////////////////////////////////
// Model World Node Class
/////////////////////////////////////////////////////////////////////////////////////

ApeModelNode *ape_model_node_create( ApeWorldNode *parent, const char *name, const char *path )
{
	ApeModel *model = ape_model_load( path );
	if ( model == nullptr )
	{
		ape_warning_( "Failed to load the specified model (%s) for node!\n", path );
		return nullptr;
	}

	ApeModelNode *modelNode = PL_NEW( ApeModelNode );
	ape_world_node_setup_( APE_WORLD_NODE( modelNode ), parent, APE_WORLD_NODE_TYPE_MODEL, name, &pl_vecOrigin3, &pl_vecOrigin3 );

	modelNode->model = model;
	PlSetupPath( modelNode->modelPath, true, "%s", path );
	modelNode->base.bounds      = compute_model_bounds( modelNode->model );
	modelNode->base.localBounds = modelNode->base.bounds;

	return modelNode;
}

static void destroy_model_node( void *data, ApeWorldNode *parent )
{
	ApeModelNode *self = ( ApeModelNode * ) data;

	ape_model_release( self->model );

	PL_DELETE( self );
}

AcmBranch *serialize_model_node( void *data, AcmBranch *root )
{
	ApeModelNode *self = ( ApeModelNode * ) data;
	acm_push_string( root, "path", self->modelPath, true );

	return root;
}

ApeWorldNode *deserialize_model_node( ApeWorldNode *parent, AcmBranch *root )
{
	PLPath modelPath;
	PlSetupPath( modelPath, true, "%s", acm_get_string( root, "path", "" ) );
	ApeModelNode *modelNode = ape_model_node_create( parent, nullptr, modelPath );

	return ( ApeWorldNode * ) modelNode;
}

const ApeWorldNodeClass ape_modelClass = {
        .identifier          = "model",
        .magic               = PL_MAGIC_TO_NUM( 'M', 'O', 'D', 'L' ),
        .destroyFunction     = destroy_model_node,
        .serializeFunction   = serialize_model_node,
        .deserializeFunction = deserialize_model_node,
};
