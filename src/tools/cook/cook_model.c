// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cooking methods specific to models.
// Author:  Mark E. Sowden

#include "cook.h"

#include "model/model_obj.h"

#include "ape/ape_formats.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

void cook_model_process( const char *modelName )
{
	PLPath path;
	PlSetupPath( path, true, "models/%s", modelName );

	const char *extension = PlGetFileExtension( modelName );
	if ( extension == NULL )
		ERROR( "Failed to get extension for model (%s): %s\n", modelName, PlGetError() );

	NdBranch *root = ndPushBackObject( NULL, "model" );
	ndPushBackUI32( root, "version", SS_APE_FORMAT_MODEL_VERSION );

	if ( pl_strcasecmp( extension, "smd" ) == 0 && !model_smd_serialize( root, path ) )
		ERROR( "Failed to serialize smd model (%s)!\n", path )
	else if ( pl_strcasecmp( extension, "obj" ) == 0 && !model_obj_serialize( root, path ) )
		ERROR( "Failed to serialize obj model (%s)!\n", path )

	PlSetupPath( path, true, "%s/ship/models/%s." SS_APE_FORMAT_MODEL_EXTENSION, ss_com_project_get_local_path(), modelName );
	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
		ERROR( "Failed to write model: %s\n", ndGetErrorMessage() );
}
