// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cooking methods specific to models.
// Author:  Mark E. Sowden

#include "plcore/pl_hashtable.h"
#include "plcore/pl_timer.h"

#include "cook.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

extern const CookModelFormatInterface  modelSmdInterface;
extern const CookModelFormatInterface  modelObjInterface;
static const CookModelFormatInterface *modelCookFormats[] = {
        &modelObjInterface,
        &modelSmdInterface,
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

static void deserialize_model_animations( AcmBranch *root, ApeFormatModel *dst, const char *folder )
{
}

static void parse_model_config( AcmBranch *root, ApeFormatModel *dst, const char *folder )
{
	const char *name = acm_branch_get_child_string( root, "name", nullptr );
	if ( name == nullptr )
	{
		WARN( "No name specified for model!\n" );
		return;
	}

	const char *materialPath = acm_branch_get_child_string( root, "materialPath", folder );
	PlSetupPath( dst->materialPath, true, "%s", materialPath );

	const char *body = acm_branch_get_child_string( root, "body", nullptr );
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

		const CookModelFormatInterface **interface = modelCookFormats;
		while ( ( *interface ) != nullptr )
		{
			if ( ( *interface )->extension != nullptr && ( pl_strcasecmp( ( *interface )->extension, ext ) == 0 ) )
			{
				assert( ( *interface )->loadFunction );
				assert( ( *interface )->convertFunction );
				assert( ( *interface )->deleteFunction );

				CookModel *model = ( *interface )->loadFunction( bodyPath );
				if ( model == nullptr )
				{
					interface++;
					continue;
				}

				( *interface )->convertFunction( model, dst );
				( *interface )->deleteFunction( model );
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

	dst->isStatic = acm_branch_get_child_bool( root, "isStatic", false );
	if ( !dst->isStatic )
	{
		if ( dst->numBones > 0 )
		{
			AcmBranch *child;
			if ( ( child = acm_branch_get_child_by_name( root, "animations" ) ) != nullptr )
			{
				deserialize_model_animations( child, dst, folder );
			}
			else
			{
				WARN( "Skeletal model, but no animations specified!\n" );
			}
		}
	}

	AcmBranch *child;
	if ( ( child = acm_branch_get_child_by_name( root, "attachments" ) ) != nullptr )
	{
	}

	snprintf( dst->name, sizeof( dst->name ), "%s", name );
}

typedef struct VectorIndex
{
	const PLVector3 *vec;
	unsigned int     pos;
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

static void serialize_triangle( AcmBranch *root, const ApeFormatTriangle *triangle, const ApeFormatVertex *vertices, PLHashTable *vertexTable, PLHashTable *normalsTable )
{
	AcmBranch *triangleBranch = acm_branch_push_back_object( root, nullptr );

	const ApeFormatVertex *a = &vertices[ triangle->indices[ 0 ] ];
	const ApeFormatVertex *b = &vertices[ triangle->indices[ 1 ] ];
	const ApeFormatVertex *c = &vertices[ triangle->indices[ 2 ] ];

	unsigned int vertexIndices[ 3 ] = {
	        get_vector_index( &a->position, vertexTable ),
	        get_vector_index( &b->position, vertexTable ),
	        get_vector_index( &c->position, vertexTable ),
	};
	acm_branch_push_back_uint32_array( triangleBranch, "vertex", vertexIndices, 3 );

	unsigned int normalIndices[ 3 ] = {
	        get_vector_index( &a->normal, normalsTable ),
	        get_vector_index( &b->normal, normalsTable ),
	        get_vector_index( &c->normal, normalsTable ),
	};
	acm_branch_push_back_uint32_array( triangleBranch, "normal", normalIndices, 3 );
}

static void serialize_mesh( AcmBranch *root, const ApeFormatMesh *mesh, const ApeFormatVertex *vertices, PLHashTable *vertexTable, PLHashTable *normalsTable )
{
	char *c = strrchr( mesh->material, '/' );
	printf( "\tSerialising mesh (%s)\n", c != nullptr ? ( c + 1 ) : mesh->material );

	AcmBranch *meshBranch = acm_branch_push_back_object( root, nullptr );

	//TODO: should go ahead and ensure material exists, and associated texture for material is cooked, etc.
	acm_branch_push_back_string( meshBranch, "material", mesh->material );

	AcmBranch *trianglesBranch = acm_branch_push_back_object_array( meshBranch, "triangles" );
	printf( "\t\t%u triangles\n", mesh->numTriangles );
	for ( unsigned int i = 0; i < mesh->numTriangles; ++i )
	{
		serialize_triangle( trianglesBranch, &mesh->triangles[ i ], vertices, vertexTable, normalsTable );
	}
}

static void serialize_bone( AcmBranch *root, const ApeFormatBone *bone )
{
	printf( "\tSerialising bone (%s)\n", bone->name );

	AcmBranch *boneBranch = acm_branch_push_back_object( root, nullptr );
	acm_branch_push_back_string( boneBranch, "name", bone->name );
	acm_branch_push_back_uint32( boneBranch, "parent", bone->parent );
	acm_branch_push_back_float32_array( boneBranch, "position", ( float * ) &bone->position, 3 );
	acm_branch_push_back_float32_array( boneBranch, "rotation", ( float * ) &bone->rotation, 3 );
}

static AcmBranch *serialize_ape_format_model( const ApeFormatModel *model )
{
	AcmBranch *root = acm_branch_push_back_object( nullptr, "model" );

	acm_branch_push_back_uint32( root, "version", APE_FORMAT_MODEL_VERSION );

	// build up lists of unique vertex data sets...
	PLHashTable *vertexTable = PlCreateHashTable();
	assert( model->numVertices > 0 );
	printf( "%u vertices\n", model->numVertices );
	for ( unsigned int i = 0; i < model->numVertices; ++i )
	{
		if ( PlLookupHashTableUserData( vertexTable, &model->vertices[ i ].position, sizeof( PLVector3 ) ) != nullptr )
		{
			continue;
		}

		VectorIndex *index = PL_NEW( VectorIndex );
		index->pos         = i;
		index->vec         = &model->vertices[ i ].position;
		PlInsertHashTableNode( vertexTable, &model->vertices[ i ].position, sizeof( PLVector3 ), index );
	}
	//todo: we might as well encode the normals into the same table... can't remember why we didn't!
	PLHashTable *normalsTable = PlCreateHashTable();
	for ( unsigned int i = 0; i < model->numVertices; ++i )
	{
		if ( PlLookupHashTableUserData( vertexTable, &model->vertices[ i ].normal, sizeof( PLVector3 ) ) != nullptr )
		{
			continue;
		}

		VectorIndex *index = PL_NEW( VectorIndex );
		index->pos         = i;
		index->vec         = &model->vertices[ i ].normal;
		PlInsertHashTableNode( normalsTable, &model->vertices[ i ].normal, sizeof( PLVector3 ), index );
	}

	AcmBranch       *branch;
	PLHashTableNode *childHashNode;

	branch        = acm_branch_push_back_float32_array( root, "vertices", nullptr, 0 );
	childHashNode = PlGetFirstHashTableNode( vertexTable );
	while ( childHashNode != nullptr )
	{
		const PLVector3 *v = ( ( VectorIndex * ) ( PlGetHashTableNodeUserData( childHashNode ) ) )->vec;
		acm_branch_push_back_float32( branch, nullptr, v->x );
		acm_branch_push_back_float32( branch, nullptr, v->y );
		acm_branch_push_back_float32( branch, nullptr, v->z );
		childHashNode = PlGetNextHashTableNode( childHashNode );
	}

	branch        = acm_branch_push_back_float32_array( root, "normals", nullptr, 0 );
	childHashNode = PlGetFirstHashTableNode( normalsTable );
	while ( childHashNode != nullptr )
	{
		const PLVector3 *v = ( ( VectorIndex * ) ( PlGetHashTableNodeUserData( childHashNode ) ) )->vec;
		acm_branch_push_back_float32( branch, nullptr, v->x );
		acm_branch_push_back_float32( branch, nullptr, v->y );
		acm_branch_push_back_float32( branch, nullptr, v->z );
		childHashNode = PlGetNextHashTableNode( childHashNode );
	}

	acm_branch_push_back_bool( root, "isStatic", model->isStatic );
	if ( model->numBones > 0 )
	{
		branch = acm_branch_push_back_object_array( root, "bones" );
		for ( unsigned int i = 0; i < model->numBones; ++i )
		{
			serialize_bone( branch, &model->bones[ i ] );
		}
	}

	printf( "%u meshes\n", model->numMeshes );
	branch = acm_branch_push_back_object_array( root, "meshes" );
	for ( unsigned int i = 0; i < model->numMeshes; ++i )
	{
		serialize_mesh( branch, &model->meshes[ i ], model->vertices, vertexTable, normalsTable );
	}

	PlDestroyHashTableEx( normalsTable, PlFree );
	PlDestroyHashTableEx( vertexTable, PlFree );

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
	AcmBranch *root = serialize_ape_format_model( model );
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

	if ( !acm_write_file( path, root, ND_FILE_BINARY ) )
	{
		WARN( "Failed to write model file: %s\n", acm_get_error_message() );
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

	AcmBranch *root = acm_load_file( path, "cookModel" );
	if ( root != nullptr )
	{
		double startTime = PlGetCurrentSeconds();

		parse_model_config( root, model, folder );

		acm_branch_destroy( root );

		write_ape_format_model( model, folder );

		double endTime = PlGetCurrentSeconds();

		printf( "Processed model in %.2lfs\n", endTime - startTime );
	}
	else
	{
		WARN( "Failed to open model cook file (%s): %s\n", path, acm_get_error_message() );
	}

	PL_DELETE( model );
}
