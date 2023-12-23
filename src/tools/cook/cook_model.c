// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cooking methods specific to models.
// Author:  Mark E. Sowden

#include "cook.h"

#include "model/model.h"
#include "model/model_obj.h"

#include "ape/ape_formats.h"

#include "plcore/pl_hashtable.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static bool serialize_ape_model( NdBranch *root, const SSApeFormatModel *model )
{
	if ( model->numMeshes == 0 )
	{
		WARN( "Attempted to serialize an empty model!\n" );
		return false;
	}

	NdBranch *materialsBranch = ndPushBackStringArray( root, "materials", NULL, 0 );
	for ( unsigned int i = 0; i < model->numMeshes; ++i )
		ndPushBackString( materialsBranch, NULL, model->meshes[ i ].material );

	PLHashTable *uniqueVertices = PlCreateHashTable();

	PlDestroyHashTable( uniqueVertices );

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

extern CookModelFormatInterface modelSmdInterface;
extern CookModelFormatInterface modelObjInterface;

static const CookModelFormatInterface *modelCookFormats[] = {
        &modelObjInterface,
        &modelSmdInterface,
        NULL,
};

void cook_model_process( const char *modelName )
{
	PLPath path;
	PlSetupPath( path, true, "models/%s", modelName );

	const char *extension = PlGetFileExtension( modelName );
	if ( extension == NULL )
		ERROR( "Failed to get extension for model (%s): %s\n", modelName, PlGetError() );

	NdBranch *root = ndPushBackObject( NULL, "model" );
	ndPushBackUI32( root, "version", SS_APE_FORMAT_MODEL_VERSION );

	SSApeFormatModel model = {};

	for ( unsigned int i = 0;; ++i )
	{
		if ( modelCookFormats[ i ] == NULL )
			ERROR( "Unsupported model format (%s)!\n", path );

		if ( pl_strcasecmp( extension, modelCookFormats[ i ]->extension ) != 0 )
			continue;

		void *pm = modelCookFormats[ i ]->loadFunction( path );
		if ( pm == NULL )
			ERROR( "Failed to load model (%s)!\n", path );

		if ( modelCookFormats[ i ]->convertFunction( pm, &model ) == NULL )
			ERROR( "Failed to convert model (%s)!\n", path );

		modelCookFormats[ i ]->deleteFunction( pm );
		break;
	}

	if ( !serialize_ape_model( root, &model ) )
		ERROR( "Failed to serialize model!\n" );

	PlSetupPath( path, true, "%s/ship/models/%s." SS_APE_FORMAT_MODEL_EXTENSION, ss_com_project_get_local_path(), modelName );
	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
		ERROR( "Failed to write model: %s\n", ndGetErrorMessage() );
}
