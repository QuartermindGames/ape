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

static void deserialize_model_config( NdBranch *root, ApeFormatModel *dst )
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

extern CookModelFormatInterface modelSmdInterface;
extern CookModelFormatInterface modelObjInterface;
extern CookModelFormatInterface modelHowPCInterface;
static const CookModelFormatInterface *modelCookFormats[] = {
        &modelObjInterface,
        &modelSmdInterface,
        &modelHowPCInterface,
        NULL,
};

void cook_model_process( const char *modelName )
{
	PLPath path;
	PlSetupPath( path, true, "models/%s", modelName );

#if 1

	ApeFormatModel model = {};
	NdBranch *root = nd_load_file( path, "modelConvert" );
	if ( root == NULL )
	{

	}

#else // old

	const char *extension = PlGetFileExtension( modelName );
	if ( extension == NULL )
	{
		ERROR( "Failed to get extension for model (%s): %s\n", modelName, PlGetError() );
	}

	ApeFormatModel model = {};

	for ( unsigned int i = 0;; ++i )
	{
		if ( modelCookFormats[ i ] == NULL )
		{
			ERROR( "Unsupported model format (%s)!\n", path );
		}

		if ( pl_strcasecmp( extension, modelCookFormats[ i ]->extension ) != 0 )
		{
			continue;
		}

		void *pm = modelCookFormats[ i ]->loadFunction( path );
		if ( pm == NULL )
		{
			ERROR( "Failed to load model (%s)!\n", path );
		}

		if ( modelCookFormats[ i ]->convertFunction( pm, &model ) == NULL )
		{
			ERROR( "Failed to convert model (%s)!\n", path );
		}

		modelCookFormats[ i ]->deleteFunction( pm );
		break;
	}

	NdBranch *root = serialize_ape_model( &model );
	if ( root == NULL )
	{
		ERROR( "Failed to serialize model!\n" );
	}

	PlSetupPath( path, true, "%s/ship/models/%s." APE_FORMAT_MODEL_EXTENSION, com_project_get_local_path(), modelName );
	if ( !nd_write_file( path, root, ND_FILE_BINARY ) )
	{
		ERROR( "Failed to write model: %s\n", nd_get_error_message() );
	}

#endif
}
