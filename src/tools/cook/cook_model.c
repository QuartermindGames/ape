// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cooking methods specific to models.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_time.h"

#include "plcore/pl_hashtable.h"
#include "plcore/pl_timer.h"
#include <plcore/pl_filesystem.h>

#include "cook.h"
#include "model/model.h"

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

static void deserialize_model_animations( AcmBranch *root, CookModel *dst, const char *folder )
{
}

static void parse_model_config( AcmBranch *root, CookModel *dst, const char *folder )
{
	const char *name = acm_get_string( root, "name", nullptr );
	if ( name == nullptr )
	{
		WARN( "No name specified for model!\n" );
		return;
	}

	const char *materialPath = acm_get_string( root, "materialPath", folder );
	PlSetupPath( dst->materialPath, true, "%s", materialPath );

	dst->scale = acm_get_f32( root, "scale", 1.0f );

	const char *body = acm_get_string( root, "body", nullptr );
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
		while ( *interface != nullptr )
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

		// apply the scale here...
		for ( unsigned int i = 0; i < dst->numVertices; ++i )
		{
			dst->vertices[ i ].position = PlScaleVector3F( dst->vertices[ i ].position, dst->scale );
			dst->vertices[ i ].normal   = PlScaleVector3F( dst->vertices[ i ].normal, dst->scale );
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

	dst->isStatic = acm_get_bool( root, "isStatic", false );
	if ( !dst->isStatic )
	{
		if ( dst->numBones > 0 )
		{
			AcmBranch *child;
			if ( ( child = acm_get_child_by_name( root, "animations" ) ) != nullptr )
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
	if ( ( child = acm_get_child_by_name( root, "attachments" ) ) != nullptr )
	{
	}

	snprintf( dst->name, sizeof( dst->name ), "%s", name );
}

#if 0// originally had a bunch of fancy crap for only storing unique vertices, but the complexity doesn't seem worth it
typedef struct VectorIndex
{
	const PLVector3 *vec;
	unsigned int             pos;
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
#endif

static void serialize_mesh( AcmBranch *root, const CookModelMesh *mesh, const CookModelVertex *vertices )
{
	char *c = strrchr( mesh->material, '/' );
	printf( "\tSerialising mesh (%s)\n", c != nullptr ? c + 1 : mesh->material );

	AcmBranch *meshBranch = acm_push_object( root, nullptr );

	//TODO: should go ahead and ensure material exists, and associated texture for material is cooked, etc.
	acm_push_string( meshBranch, "material", mesh->material, false );

	AcmBranch *trianglesBranch = acm_push_array_object( meshBranch, "triangles" );
	printf( "\t\t%u triangles\n", mesh->numTriangles );
	for ( unsigned int i = 0; i < mesh->numTriangles; ++i )
	{
		AcmBranch *triangleBranch = acm_push_object( trianglesBranch, nullptr );
		acm_push_array_ui32( triangleBranch, "vertex", mesh->triangles[ i ].indices, 3 );
	}
}

static void serialize_bone( AcmBranch *root, const ApeFormatBone *bone, const CookModel *model )
{
	//printf( "\tSerialising bone (%s)\n", bone->name );

	AcmBranch *boneBranch = acm_push_object( root, nullptr );
	acm_push_string( boneBranch, "name", bone->name, false );
	if ( bone->parent > -1 )
	{
		acm_push_i32( boneBranch, "parent", bone->parent );
	}

	PLVector3 bonePosition = PlScaleVector3F( bone->position, model->scale );
	acm_push_array_f32( boneBranch, "position", ( float * ) &bonePosition, 3 );
	acm_push_array_f32( boneBranch, "rotation", ( float * ) &bone->rotation, 3 );
}

static AcmBranch *serialize_ape_format_model( const CookModel *model )
{
	AcmBranch *root = acm_push_object( nullptr, "model" );

	acm_push_ui32( root, "version", APE_FORMAT_MODEL_VERSION );

	//TODO: this is pretty basic and hard-coded for now... meh...
	//TODO: allow us to store vertex data as other data types besides floats...
	AcmBranch *branch = acm_push_object( root, "vertexFormatDescriptor" );
	acm_push_ui32( branch, "numFloatElements", 8 );
	acm_push_bool( branch, "hasPosition", true );
	acm_push_bool( branch, "hasNormal", true );
	acm_push_bool( branch, "hasUV", true );

	branch = acm_push_array_f32( root, "vertices", nullptr, 0 );
	for ( unsigned int i = 0; i < model->numVertices; ++i )
	{
		const CookModelVertex *vertexIndex = &model->vertices[ i ];
		acm_push_f32( branch, nullptr, vertexIndex->position.x );
		acm_push_f32( branch, nullptr, vertexIndex->position.y );
		acm_push_f32( branch, nullptr, vertexIndex->position.z );
		acm_push_f32( branch, nullptr, vertexIndex->normal.x );
		acm_push_f32( branch, nullptr, vertexIndex->normal.y );
		acm_push_f32( branch, nullptr, vertexIndex->normal.z );
		acm_push_f32( branch, nullptr, vertexIndex->uv.x );
		acm_push_f32( branch, nullptr, vertexIndex->uv.y );
	}

	acm_push_bool( root, "isStatic", model->isStatic );
	if ( model->numBones > 0 )
	{
		printf( "%u bones\n", model->numBones );
		branch = acm_push_array_object( root, "bones" );
		for ( unsigned int i = 0; i < model->numBones; ++i )
		{
			serialize_bone( branch, &model->bones[ i ], model );
		}
	}

	printf( "%u meshes\n", model->numMeshes );
	branch = acm_push_array_object( root, "meshes" );
	for ( unsigned int i = 0; i < model->numMeshes; ++i )
	{
		serialize_mesh( branch, &model->meshes[ i ], model->vertices );
	}

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

static void write_ape_format_model( const CookModel *model, const char *folder )
{
	AcmBranch *root = serialize_ape_format_model( model );
	if ( root == nullptr )
	{
		WARN( "Failed to serialize model!\n" );
		return;
	}

	PLPath path = {};
	PlSetupPath( path, true, "%s/%s." APE_FORMAT_MODEL_EXTENSION, folder, model->name );
	if ( !create_file_path( path ) )
	{
		return;
	}

	if ( !acm_write_file( path, root, ACM_FILE_TYPE_BINARY ) )
	{
		WARN( "Failed to write model file: %s\n", acm_get_error_message() );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void cook_model_process( const char *modelName )
{
	PLPath path = {};
	if ( PlResolveVirtualPath( modelName, path, sizeof( path ) ) == nullptr )
	{
		WARN( "Failed to resolve virtual path: %s\n", PlGetError() );
		return;
	}

	CookModel *model = PL_NEW( CookModel );
	AcmBranch *root  = acm_load_file( path, "cookModel" );
	if ( root != nullptr )
	{
		double startTime = qm_os_time_get_seconds();

		PLPath folder = {};
		if ( PlGetFolderForPath( folder, path ) == nullptr )
		{
			WARN( "Failed to get folder from path (%s)!\n", path );
			return;
		}

		parse_model_config( root, model, folder );

		acm_branch_destroy( root );

		write_ape_format_model( model, folder );

		double endTime = qm_os_time_get_seconds();

		printf( "Processed model in %.2lfs\n", endTime - startTime );
	}
	else
	{
		WARN( "Failed to open model cook file (%s): %s\n", path, acm_get_error_message() );
	}

	PL_DELETE( model );
}
