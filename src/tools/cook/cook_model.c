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

static void deserialize_model_config( NdBranch *root, ApeFormatModel *dst )
{
	const char *bodyName = nd_branch_get_child_string( root, "body", nullptr );
	if ( bodyName != nullptr )
	{
		const char *ext = PlGetFileExtension( bodyName );
		if ( ext == nullptr )
		{
			return;
		}

		const CookModelFormatInterface *interface = ( const CookModelFormatInterface * ) &modelCookFormats[ 0 ];
		while ( interface != nullptr )
		{
			if ( pl_strcasecmp( interface->extension, ext ) != 0 )
			{
				interface++;
				continue;
			}

			if ( interface->loadFunction != nullptr && interface->loadFunction( bodyName ) != nullptr )
			{
				break;
			}

			interface++;
		}

		if ( interface == nullptr )
		{
			WARN( "Failed to find appropriate interface for format (%s)!\n", ext );
			return;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void cook_model_process( const char *modelName )
{
	PLPath path;
	PlSetupPath( path, true, "models/%s.%s", modelName, COOK_MODEL_EXTENSION );

	ApeFormatModel *model = PL_NEW( ApeFormatModel );

	NdBranch *root = nd_load_file( path, "cookModel" );
	if ( root == nullptr )
	{
		WARN( "Failed to open model cook file (%s): %s\n", path, nd_get_error_message() );
		return;
	}

	deserialize_model_config( root, model );

	nd_branch_destroy( root );

	PL_DELETE( model );
}
