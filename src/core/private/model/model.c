// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Model deserialization and caching.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"

#include "ape_private.h"
#include "model.h"
#include "world/world.h"
#include "renderer/renderer.h"

static bool modelShowSkeleton;

APE_MEMORY_IMPLEMENT_INTERFACE( ape_model, ApeModel, reference )

void ape_model_register_console_variables_()
{
	ape_console_var_register( "model.showSkeleton", "Toggle display of a model skeleton.", "false", PL_VAR_BOOL, &modelShowSkeleton, nullptr, 0 );
}

static void model_cleanup_callback_( void *userData )
{
	ApeModel *model = userData;
	assert( model != NULL );

	for ( unsigned int i = 0; i < model->numMaterials; ++i )
	{
		ape_material_release_reference( model->meshes[ i ].material );
	}

	PlDestroyLinkedList( model->sceneNodes );

	qm_gfx_mesh_destroy( model->cache );

	qm_os_memory_free( model );
}

static ApeModelMesh *deserialize_mesh( ApeModel *model, ApeModelMesh *mesh, AcmBranch *root )
{
	const char *materialPath = acm_get_string( root, "material", nullptr );
	if ( materialPath == nullptr )
	{
		ape_console_warning_( "No material provided for mesh!\n" );
		return nullptr;
	}

	mesh->material = ape_material_cache( materialPath, APE_CACHE_GROUP_WORLD, true );

	mesh->startIndex = model->cache->num_indices;

	AcmBranch *branch;
	if ( ( branch = acm_get_child_by_name( root, "triangles" ) ) != NULL )
	{
		ACM_ITERATE_BRANCH( branch, i )
		{
			unsigned int vertexIndices[ 3 ] = {};

			AcmBranch *childBranch;
			if ( ( childBranch = acm_get_child_by_name( i, "vertex" ) ) != nullptr )
			{
				acm_branch_get_uint32_array( childBranch, vertexIndices, 3 );
			}

			PlgAddMeshTriangle( model->cache, vertexIndices[ 0 ], vertexIndices[ 1 ], vertexIndices[ 2 ] );
		}

		mesh->endIndex = model->cache->num_indices;
	}
	else
	{
		ape_console_warning_( "Mesh has no indices!\n" );
	}

	return mesh;
}

static ApeModel *deserialize_model( ApeModel *model, AcmBranch *root )
{
	unsigned int version = acm_get_uint( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 || version > APE_FORMAT_MODEL_VERSION )
	{
		ape_console_warning_( "Invalid model version, %d, expected %u!\n", version, APE_FORMAT_MODEL_VERSION );
		return nullptr;
	}

	AcmBranch *branch;

	unsigned int numFloatElements;
	if ( ( branch = acm_get_child_by_name( root, "vertexFormatDescriptor" ) ) != nullptr )
	{
		numFloatElements = acm_get_uint( branch, "numFloatElements", 0 );
		if ( numFloatElements == 0 )
		{
			ape_console_warning_( "Invalid number of float elements per vertex descriptor!\n" );
			return nullptr;
		}
	}
	else
	{
		ape_console_warning_( "Mesh has no vertex descriptor!\n" );
		return nullptr;
	}

	float       *vertices    = nullptr;
	unsigned int numVertices = 0;
	if ( ( branch = acm_get_child_by_name( root, "vertices" ) ) != nullptr )
	{
		unsigned int numIndices = acm_get_num_of_children( branch );
		if ( numIndices >= 3 )
		{
			vertices    = QM_OS_MEMORY_NEW_( float, numIndices );
			numVertices = numIndices / numFloatElements;
			acm_branch_get_float32_array( branch, vertices, numIndices );
		}
		else
		{
			ape_console_warning_( "Invalid number (%u) of positions in model!\n", numVertices );
			numVertices = 0;
		}
	}

	if ( numVertices == 0 )
	{
		ape_console_warning_( "Mesh has no vertices!\n" );
		return nullptr;
	}

	model->cache = qm_gfx_mesh_create( QM_GFX_MESH_PRIMITIVE_TRIANGLES, QM_GFX_MESH_DRAW_MODE_STATIC, 0, numVertices );
	if ( model->cache == nullptr )
	{
		ape_console_warning_( "Failed to create cache for model: %s\n", PlGetError() );
		return nullptr;
	}

	const float *v = vertices;
	for ( unsigned int i = 0; i < numVertices; ++i, v += numFloatElements )
	{
		PlgAddMeshVertex( model->cache,
		                  ( const QmMathVector3f * ) &v[ 0 ],
		                  ( const QmMathVector3f * ) &v[ 3 ], &PL_COLOUR_WHITE,
		                  ( const QmMathVector2f * ) &v[ 6 ] );
	}

	AcmBranch *meshArray = acm_get_child_by_name( root, "meshes" );
	if ( meshArray == NULL || ( ( model->numMaterials = acm_get_num_of_children( meshArray ) ) == 0 ) )
	{
		ape_console_warning_( "No meshes for model!\n" );
		return nullptr;
	}
	if ( model->numMaterials >= IO_MODEL_MAX_MATERIALS )
	{
		ape_console_warning_( "Unexpected number of meshes (%u >= %u)!\n", model->numMaterials, IO_MODEL_MAX_MATERIALS );
		model->numMaterials = IO_MODEL_MAX_MATERIALS - 1;
	}

	if ( meshArray != nullptr )
	{
		AcmBranch *meshNode = acm_get_first_child( meshArray );
		for ( unsigned int i = 0; i < model->numMaterials; ++i )
		{
			assert( meshNode != NULL );
			if ( deserialize_mesh( model, &model->meshes[ i ], meshNode ) == nullptr )
			{
				ape_console_warning_( "Failed to deserialize mesh %u!\n", i );
				break;
			}

			meshNode = acm_get_next_child( meshNode );
		}
	}

	AcmBranch *bonesList = acm_get_child_by_name( root, "bones" );
	if ( bonesList != NULL )
	{
		model->numBones = acm_get_num_of_children( bonesList );
		if ( model->numBones >= IO_MODEL_MAX_BONES )
		{
			ape_console_warning_( "Unexpected number of bones (%u >= %u)!", model->numBones, IO_MODEL_MAX_BONES );
			model->numBones = ( IO_MODEL_MAX_BONES - 1 );
		}

		AcmBranch *child = acm_get_first_child( bonesList );
		for ( unsigned int i = 0; i < model->numBones; ++i )
		{
			if ( child == NULL )
			{
				break;
			}

			model->bones[ i ].parent   = acm_get_int( child, "parent", -1 );
			model->bones[ i ].position = com_acm_get_vector3( child, "position", &QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ) );
			model->bones[ i ].rotation = com_acm_get_vector3( child, "rotation", &QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ) );

			child = acm_get_next_child( child );
		}

		unsigned int rootBone = acm_get_uint( root, "rootBone", 0 );
		if ( rootBone >= model->numBones )
		{
			ape_console_warning_( "Invalid root bone (%u), defaulting to 0!\n", rootBone );
			rootBone = 0;
		}
		model->rootBone = &model->bones[ rootBone ];
	}

	qm_os_memory_free( vertices );

	model->sceneNodes = PlCreateLinkedList();

	return model;
}

ApeModel *ape_model_load( const char *path )
{
	ApeModel *model = ape_memory_get_from_pool_( path, APE_CACHE_POOL_MODELS );
	if ( model != NULL )
	{
		ape_memory_reference_add( &model->reference );
		return model;
	}

	AcmBranch *root = com_acm_load_file( path, "model" );
	if ( root == NULL )
	{
		ape_console_warning_( "Invalid model: %s (%s)\n", acm_get_error_message(), path );
		return nullptr;
	}

	model = QM_OS_MEMORY_NEW( ApeModel );
	if ( deserialize_model( model, root ) != nullptr )
	{
		ape_memory_setup_reference( path, APE_CACHE_POOL_MODELS, &model->reference, model_cleanup_callback_, model );
		ape_memory_reference_add( &model->reference );
	}
	else
	{
		qm_os_memory_free( model );
		model = nullptr;

		ape_console_warning_( "Failed to load model, \"%s\"!\n", path );
	}

	acm_branch_destroy( root );

	return model;
}

static QmMathVector3f get_transformed_bone_position( const ApeModel *model, const IOModelBone *bone, const PLMatrix4 *transform )
{
	QmMathVector3f pos = bone->position;
	while ( bone->parent != -1 )
	{
		const IOModelBone *parent = &model->bones[ bone->parent ];

		pos  = PlTransformVector3( &pos, transform );
		pos  = qm_math_vector3f_add( pos, parent->position );
		bone = parent;
	}

	return pos;
}

void ape_model_draw( const ApeModel *model, const ApeModelAnimationState *state, const PLMatrix4 *transform, const ApeRendererPassState *passState )
{
	qm_gfx_debug_push_group_marker( "Draw Model" );

	if ( modelShowSkeleton )
	{
		for ( unsigned int i = 0; i < model->numBones; ++i )
		{
			const PLMatrix4 *mat = PlGetMatrix( PL_MODELVIEW_MATRIX );

			const IOModelBone   *a  = &model->bones[ i ];
			const QmMathVector3f pa = get_transformed_bone_position( model, a, mat );
			ape_draw_debug_sphere( pa, PL_COLOUR_CYAN, 1.0f );

			if ( model->bones[ i ].parent == -1 )
			{
				continue;
			}

			const IOModelBone   *b  = &model->bones[ model->bones[ i ].parent ];
			const QmMathVector3f pb = get_transformed_bone_position( model, b, mat );
			ape_draw_debug_arrow( pa, pb, PL_COLOUR_WHITE, 2.0f );
		}

		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( transform );

	for ( unsigned int i = 0; i < model->numMaterials; ++i )
	{
		model->cache->start = model->meshes[ i ].startIndex;
		model->cache->range = model->meshes[ i ].endIndex - model->meshes[ i ].startIndex;

		ape_material_draw( model->meshes[ i ].material, model->cache, passState );
	}

	PlPopMatrix();

	qm_gfx_debug_pop_group_marker();
}

void ape_model_draw_instanced( ApeModel *model, const PLMatrix4 **transforms, unsigned int numTransforms )
{
	//TODO: only will work with static models for now...
}

bool ape_model_is_static( const ApeModel *model )
{
	return !( model->flags & IO_MODEL_FLAG_ANIMATED );
}

static PLCollisionAABB compute_model_bounds( const ApeModel *model )
{
	PLCollisionAABB bounds = {};
	assert( model->cache->num_verts > 0 );
	float max = model->cache->vertices[ 0 ].position.x;
	float min = model->cache->vertices[ 0 ].position.x;
	for ( unsigned int i = 0; i < model->cache->num_verts; ++i )
	{
		if ( model->cache->vertices[ i ].position.x > max ) max = model->cache->vertices[ i ].position.x;
		if ( model->cache->vertices[ i ].position.y > max ) max = model->cache->vertices[ i ].position.y;
		if ( model->cache->vertices[ i ].position.z > max ) max = model->cache->vertices[ i ].position.z;
		if ( model->cache->vertices[ i ].position.x < min ) min = model->cache->vertices[ i ].position.x;
		if ( model->cache->vertices[ i ].position.y < min ) min = model->cache->vertices[ i ].position.y;
		if ( model->cache->vertices[ i ].position.z < min ) min = model->cache->vertices[ i ].position.z;
	}

	if ( min * -1 > max ) max = min * -1;
	bounds.mins = qm_math_vector3f( -max, -max, -max );
	bounds.maxs = qm_math_vector3f( max, max, max );

	return bounds;
}

static void ape_model_compute_lighting( ApeModelNode *sceneNode, double delta )
{
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( sceneNode ) );
	if ( room == nullptr || APE_WORLD_NODE( sceneNode )->flags == APE_WORLD_NODE_FLAG_HIDDEN )
	{
		return;
	}

	QmMathVector3f spos = ape_world_node_get_bounds_center( APE_WORLD_NODE( sceneNode ) );

	ApeRendererLightGridSample sample = {};
	ape_room_get_light_sample( room, spos, &sample.colour, &sample.dir );

	aux_math_interpolate_angles( &sceneNode->light.dir, &sample.dir, 7.0f * delta, &sceneNode->light.dir );
	aux_math_interpolate_colour_3f16( &sceneNode->light.colour, &sample.colour, 7.0f * delta, &sceneNode->light.colour );

	sceneNode->light.ambience = QM_MATH_COLOUR4F_TO_3F( ape_room_get_ambience( room ) );
}

void ape_model_compute_models_lighting( const double delta )
{
	// fetch all the models currently cached in the scene
	PLLinkedList *models = ape_memory_get_pool_list_( APE_CACHE_POOL_MODELS );

	ApeMemoryCacheHeader *header;
	COM_ITERATE_LINKED_LIST( header, models, i )
	{
		ApeModelNode *sceneNode;
		COM_ITERATE_LINKED_LIST( sceneNode, ( ( ApeModel * ) header->userData )->sceneNodes, j )
		{
			ape_model_compute_lighting( sceneNode, delta );
		}
	}
}

void ape_model_draw_models( ApeRoom *room, const ApeCamera *camera, const ApeRendererPassState *state )
{
	COM_PROFILE_FUNCTION_START();

	qm_gfx_debug_push_group_marker( "Draw Models" );

	// fetch all the models currently cached in the scene
	PLLinkedList *models = ape_memory_get_pool_list_( APE_CACHE_POOL_MODELS );

	// now iterate over those, and then all the nodes that reference them
	ApeMemoryCacheHeader *header;
	COM_ITERATE_LINKED_LIST( header, models, i )
	{
		ApeModel *model = header->userData;

		ApeModelNode *sceneNode;
		COM_ITERATE_LINKED_LIST( sceneNode, model->sceneNodes, j )
		{
			if ( APE_WORLD_NODE( sceneNode )->room != room || APE_WORLD_NODE( sceneNode )->flags == APE_WORLD_NODE_FLAG_HIDDEN )
			{
				continue;
			}

			ApeRendererPassState newState = *state;
			newState.lighting             = sceneNode->light;

			PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( sceneNode ) );
			ape_model_draw( model, &( ApeModelAnimationState ) {}, &transform, &newState );
		}
	}

	qm_gfx_debug_pop_group_marker();

	COM_PROFILE_FUNCTION_END();
}

/////////////////////////////////////////////////////////////////////////////////////
// Model World Node Class
/////////////////////////////////////////////////////////////////////////////////////

static void *create_model_node( ApeWorldNode *parent )
{
	ApeModelNode *modelNode = QM_OS_MEMORY_NEW( ApeModelNode );
	ape_world_node_setup_( APE_WORLD_NODE( modelNode ), parent, APE_WORLD_NODE_TYPE_MODEL, nullptr, &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO );
	return modelNode;
}

static void assign_model_to_model_node( ApeModelNode *self, ApeModel *model, const char *path )
{
	self->model = model;
	PlSetupPath( self->modelPath, true, "%s", path );

	//TODO: these should be reversed!
	self->base.bounds      = compute_model_bounds( self->model );
	self->base.localBounds = self->base.bounds;

	self->modelSceneNode = PlInsertLinkedListNode( model->sceneNodes, self );
}

ApeModelNode *ape_model_node_create( ApeWorldNode *parent, const char *name, const char *path )
{
	ApeModel *model = ape_model_load( path );
	if ( model == nullptr )
	{
		ape_console_warning_( "Failed to load the specified model (%s) for node!\n", path );
		return nullptr;
	}

	ApeModelNode *modelNode = create_model_node( parent );

	ape_world_node_set_name( APE_WORLD_NODE( modelNode ), name );

	assign_model_to_model_node( modelNode, model, path );

	return modelNode;
}

static void destroy_model_node( void *data, ApeWorldNode *parent )
{
	ApeModelNode *self = data;

	PlDestroyLinkedListNode( self->modelSceneNode );

	ape_model_release_reference( self->model );

	qm_os_memory_free( self );
}

static ApeWorldNode *clone_model_node( ApeWorldNode *src )
{
	ApeModelNode *srcModelNode = ( ApeModelNode * ) src;
	ApeModelNode *dstModelNode = ape_model_node_create( src->parent, src->name, srcModelNode->modelPath );
	if ( dstModelNode == nullptr )
	{
		ape_console_warning_( "Failed to create model for duplication!\n" );
		return nullptr;
	}

	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( srcModelNode ) );
	ape_world_node_set_position( APE_WORLD_NODE( dstModelNode ), &pos );

	QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( srcModelNode ) );
	ape_world_node_set_angles( APE_WORLD_NODE( dstModelNode ), &ang );

	return APE_WORLD_NODE( dstModelNode );
}

AcmBranch *serialize_model_node( void *data, AcmBranch *root )
{
	const ApeModelNode *self = data;
	acm_push_string( root, "path", self->modelPath, true );

	return root;
}

ApeWorldNode *deserialize_model_node( ApeWorldNode *self, AcmBranch *root )
{
	PLPath modelPath;
	PlSetupPath( modelPath, true, "%s", acm_get_string( root, "path", "" ) );

	ApeModel *model = ape_model_load( modelPath );
	if ( model == nullptr )
	{
		ape_console_warning_( "Failed to load the specified model (%s) for node!\n", modelPath );
		return nullptr;
	}

	assign_model_to_model_node( ( ApeModelNode * ) self, model, modelPath );

	return self;
}

const ApeWorldNodeClass ape_modelClass = {
        .identifier = "model",
        .magic      = QM_OS_MAGIC_TO_NUM( 'M', 'O', 'D', 'L' ),

        .create      = create_model_node,
        .destroy     = destroy_model_node,
        .serialize   = serialize_model_node,
        .deserialize = deserialize_model_node,
        .clone       = clone_model_node,

#if defined( APE_SUPPORT_EDITOR )

        .editorIcon = "resources/new_model.gif",

#endif
};
