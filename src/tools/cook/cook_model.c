// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cooking methods specific to models.
// Author:  Mark E. Sowden

#include "cook.h"

#include "model/model.h"
#include "model/model_obj.h"

#include "ape/ape_formats.h"

#include "plcore/pl_hashtable.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

extern CookModelFormatInterface modelSmdInterface;
extern CookModelFormatInterface modelObjInterface;
extern CookModelFormatInterface modelHowPCInterface;
extern CookModelFormatInterface modelPsiInterface;
static const CookModelFormatInterface *modelCookFormats[] = {
        &modelObjInterface,
        &modelSmdInterface,
        &modelHowPCInterface,
        &modelPsiInterface,
        nullptr,
};

#if 0

ND_DECLARE_STRUCT( ApeFormatBone, 2,
                   ND_DECLARE_STRUCT_ITEM( ApeFormatBone, name, ND_PROPERTY_STRING ),
                   ND_DECLARE_STRUCT_ITEM( ApeFormatBone, parent, ND_PROPERTY_UI32 ) )

ND_DECLARE_STRUCT( ApeFormatVertex, 3,
                   ND_DECLARE_STRUCT_ITEM_ARRAY( ApeFormatVertex, position, ND_PROPERTY_FLOAT32, 3 ),
                   ND_DECLARE_STRUCT_ITEM_ARRAY( ApeFormatVertex, normal, ND_PROPERTY_FLOAT32, 3 ),
                   ND_DECLARE_STRUCT_ITEM_ARRAY( ApeFormatVertex, uv, ND_PROPERTY_FLOAT32, 2 ) )

ND_DECLARE_STRUCT( ApeFormatTriangle, 1,
                   ND_DECLARE_STRUCT_ITEM_ARRAY( ApeFormatTriangle, indices, ND_PROPERTY_UI32, 3 ) )

ND_DECLARE_STRUCT( ApeFormatMesh, 2,
                   ND_DECLARE_STRUCT_ITEM( ApeFormatMesh, material, ND_PROPERTY_STRING ) )

ND_DECLARE_STRUCT( ApeFormatModel, 2,
                   ND_DECLARE_STRUCT_ITEM_ARRAY( ApeFormatModel, bones, ND_PROPERTY_OBJECT, APE_FORMAT_MODEL_MAX_BONES ),
                   ND_DECLARE_STRUCT_ITEM_ARRAY( ApeFormatModel, meshes, ND_PROPERTY_OBJECT, APE_FORMAT_MODEL_MAX_MATERIALS ) )

static NdBranch *serialize_ape_model( const ApeFormatModel *model )
{
	NdErrorCode errorCode;
	NdBranch *root = nd_serialize_struct( &ApeFormatModel_descriptor, model, &errorCode );
	if ( errorCode != ND_ERROR_SUCCESS )
	{
		nd_branch_destroy( root );
		root = NULL;
	}

	nd_branch_push_back_uint32( root, "version", APE_FORMAT_MODEL_VERSION );
	return root;
}

#endif

static void deserialize_model_animations( NdBranch *root, ApeFormatModel *dst, const char *folder )
{
}

static void deserialize_model_config( NdBranch *root, ApeFormatModel *dst, const char *folder )
{
	const char *name = nd_branch_get_child_string( root, "name", nullptr );
	if ( name == nullptr )
	{
		WARN( "No name specified for model!\n" );
		return;
	}

	const char *materialPath = nd_branch_get_child_string( root, "materialPath", folder );
	PlSetupPath( dst->materialPath, true, "%s", materialPath );

	const char *body = nd_branch_get_child_string( root, "body", nullptr );
	if ( body != nullptr )
	{
		const char *ext = PlGetFileExtension( body );
		if ( ext == nullptr )
		{
			WARN( "Failed to get extension for body (%s)!\n", body );
			return;
		}

		PLPath bodyPath;
		PlSetupPath( bodyPath, true, "%s/%s", folder, body );

		const CookModelFormatInterface *interface = ( const CookModelFormatInterface * ) modelCookFormats;
		while ( interface != nullptr )
		{
			if ( interface->extension != nullptr && ( pl_strcasecmp( interface->extension, ext ) == 0 ) )
			{
				assert( interface->loadFunction );
				assert( interface->convertFunction );
				assert( interface->deleteFunction );

				CookModel *model = interface->loadFunction( bodyPath );
				if ( model == nullptr )
				{
					interface++;
					continue;
				}

				interface->convertFunction( model, dst );
				interface->deleteFunction( model );
				break;
			}
			interface++;
		}

		if ( interface == nullptr )
		{
			WARN( "Failed to load body (%s)!\n", bodyPath );
			return;
		}
	}
	else
	{
		WARN( "No body specified for model!\n" );
		return;
	}

	NdBranch *child;
	if ( dst->numBones > 0 )
	{
		if ( ( child = nd_branch_get_child_by_name( root, "animations" ) ) != nullptr )
		{
			deserialize_model_animations( child, dst, folder );
		}
		else
		{
			WARN( "Skeletal model, but no animations specified!\n" );
		}
	}

	if ( ( child = nd_branch_get_child_by_name( root, "attachments" ) ) != nullptr )
	{
	}

	snprintf( dst->name, sizeof( dst->name ), "%s", name );
}

typedef struct VectorIndex
{
	const PLVector3 *vec;
	unsigned int pos;
} VectorIndex;

static unsigned int get_vector_index( const PLVector3 *v, PLHashTable *vectorTable )
{
	const VectorIndex *index = PlLookupHashTableUserData( vectorTable, v, sizeof( PLVector3 ) );
	if ( index == nullptr )
	{
		return ( unsigned int ) -1;
	}

	return index->pos;
}

static void serialize_triangle( NdBranch *root, const ApeFormatTriangle *triangle, const ApeFormatVertex *vertices, PLHashTable *vertexTable, PLHashTable *normalsTable )
{
	NdBranch *triangleBranch = nd_branch_push_back_object( root, nullptr );

	const ApeFormatVertex *a = &vertices[ triangle->indices[ 0 ] ];
	const ApeFormatVertex *b = &vertices[ triangle->indices[ 1 ] ];
	const ApeFormatVertex *c = &vertices[ triangle->indices[ 2 ] ];

	unsigned int vertexIndices[ 3 ] = {
	        get_vector_index( &a->position, vertexTable ),
	        get_vector_index( &b->position, vertexTable ),
	        get_vector_index( &c->position, vertexTable ),
	};
	nd_branch_push_back_uint32_array( triangleBranch, "vertex", vertexIndices, 3 );

	unsigned int normalIndices[ 3 ] = {
	        get_vector_index( &a->normal, normalsTable ),
	        get_vector_index( &b->normal, normalsTable ),
	        get_vector_index( &c->normal, normalsTable ),
	};
	nd_branch_push_back_uint32_array( triangleBranch, "normal", normalIndices, 3 );
}

static void serialize_mesh( NdBranch *root, const ApeFormatMesh *mesh, const ApeFormatVertex *vertices, PLHashTable *vertexTable, PLHashTable *normalsTable )
{
	NdBranch *meshBranch = nd_branch_push_back_object( root, nullptr );

	//TODO: should go ahead and ensure material exists, and associated texture for material is cooked, etc.
	nd_branch_push_back_string( meshBranch, "material", mesh->material );

	NdBranch *trianglesBranch = nd_branch_push_back_object_array( meshBranch, "triangles" );
	for ( unsigned int i = 0; i < mesh->numTriangles; ++i )
	{
		serialize_triangle( trianglesBranch, &mesh->triangles[ i ], vertices, vertexTable, normalsTable );
	}
}

static void serialize_bone( NdBranch *root, const ApeFormatBone *bone )
{
	NdBranch *boneBranch = nd_branch_push_back_object( root, nullptr );
	nd_branch_push_back_string( boneBranch, "name", bone->name );
	nd_branch_push_back_uint32( boneBranch, "parent", bone->parent );
	nd_branch_push_back_float32_array( boneBranch, "position", ( float * ) &bone->position, 3 );
	nd_branch_push_back_float32_array( boneBranch, "rotation", ( float * ) &bone->rotation, 3 );
}

static NdBranch *serialize_ape_format_model( const ApeFormatModel *model )
{
	NdBranch *root = nd_branch_push_back_object( nullptr, "model" );

	nd_branch_push_back_uint32( root, "version", APE_FORMAT_MODEL_VERSION );

	// build up lists of unique vertex data sets...
	PLHashTable *vertexTable = PlCreateHashTable();
	for ( unsigned int i = 0; i < model->numVertices; ++i )
	{
		VectorIndex *index = PL_NEW( VectorIndex );
		index->pos = i;
		index->vec = &model->vertices[ i ].position;
		PlInsertHashTableNode( vertexTable, &model->vertices[ i ].position, sizeof( PLVector3 ), index );
	}
	PLHashTable *normalsTable = PlCreateHashTable();
	for ( unsigned int i = 0; i < model->numVertices; ++i )
	{
		VectorIndex *index = PL_NEW( VectorIndex );
		index->pos = i;
		index->vec = &model->vertices[ i ].normal;
		PlInsertHashTableNode( normalsTable, &model->vertices[ i ].normal, sizeof( PLVector3 ), index );
	}

	NdBranch *child;
	PLHashTableNode *childHashNode;

	child = nd_branch_push_back_float32_array( root, "vertices", nullptr, 0 );
	childHashNode = PlGetFirstHashTableNode( vertexTable );
	while ( childHashNode != nullptr )
	{
		const PLVector3 *v = ( ( VectorIndex * ) ( PlGetHashTableNodeUserData( childHashNode ) ) )->vec;
		nd_branch_push_back_float32( child, nullptr, v->x );
		nd_branch_push_back_float32( child, nullptr, v->y );
		nd_branch_push_back_float32( child, nullptr, v->z );
		childHashNode = PlGetNextHashTableNode( childHashNode );
	}

	child = nd_branch_push_back_float32_array( root, "normals", nullptr, 0 );
	childHashNode = PlGetFirstHashTableNode( normalsTable );
	while ( childHashNode != nullptr )
	{
		const PLVector3 *v = ( ( VectorIndex * ) ( PlGetHashTableNodeUserData( childHashNode ) ) )->vec;
		nd_branch_push_back_float32( child, nullptr, v->x );
		nd_branch_push_back_float32( child, nullptr, v->y );
		nd_branch_push_back_float32( child, nullptr, v->z );
		childHashNode = PlGetNextHashTableNode( childHashNode );
	}

	child = nd_branch_push_back_object_array( root, "bones" );
	for ( unsigned int i = 0; i < model->numBones; ++i )
	{
		serialize_bone( child, &model->bones[ i ] );
	}

	child = nd_branch_push_back_object_array( root, "meshes" );
	for ( unsigned int i = 0; i < model->numMeshes; ++i )
	{
		serialize_mesh( child, &model->meshes[ i ], model->vertices, vertexTable, normalsTable );
	}

	PlDestroyHashTableEx( normalsTable, pl_free );
	PlDestroyHashTableEx( vertexTable, pl_free );

	return root;
}

/**
 * This basically works under the assumption that
 * the given path includes the name of a file at
 * the end.
 */
static bool create_file_path( const char *path )
{
	PLPath makePath;
	PlSetupPath( makePath, true, "%s", path );
	char *c = strrchr( makePath, '/' );
	if ( c == nullptr || *( c + 1 ) == '\0' )
	{
		// no path, so return
		return true;
	}

	*c = '\0';
	if ( !PlCreatePath( makePath ) )
	{
		WARN( "Failed to create destination path (%s): %s\n", makePath, PlGetError() );
		return false;
	}

	return true;
}

static void write_ape_format_model( const ApeFormatModel *model, const char *folder )
{
	NdBranch *root = serialize_ape_format_model( model );
	if ( root == nullptr )
	{
		WARN( "Failed to serialize model!\n" );
		return;
	}

	PLPath path = {};
	PlSetupPath( path, true, "%s/ship/models/%s." APE_FORMAT_MODEL_EXTENSION, com_project_get_local_path(), model->name );
	if ( !create_file_path( path ) )
	{
		return;
	}

	if ( !nd_write_file( path, root, ND_FILE_BINARY ) )
	{
		WARN( "Failed to write model file: %s\n", nd_get_error_message() );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void cook_model_process( const char *modelName )
{
	PLPath path = {};
	PlSetupPath( path, true, "models/%s.%s", modelName, COOK_MODEL_EXTENSION );

	PLPath folder = {};
	if ( PlGetFolderForPath( folder, path ) == nullptr )
	{
		WARN( "Failed to get folder from path (%s)!\n", path );
		return;
	}

	ApeFormatModel *model = PL_NEW( ApeFormatModel );

	NdBranch *root = nd_load_file( path, "cookModel" );
	if ( root != nullptr )
	{
		deserialize_model_config( root, model, folder );

		nd_branch_destroy( root );

		write_ape_format_model( model, folder );
	}
	else
	{
		WARN( "Failed to open model cook file (%s): %s\n", path, nd_get_error_message() );
	}

	PL_DELETE( model );
}
