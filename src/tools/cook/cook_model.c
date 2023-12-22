// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Cooking methods specific to models.
// Author:  Mark E. Sowden

#include "cook.h"

#include "model/format_obj.h"

#include "ape/ape_formats.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

//TODO: maybe consider moving this into the actual format_obj.c impl?
static bool serialize_obj( NdBranch *root, const char *sourcePath )
{
	ObjModel *model = model_obj_load( sourcePath );
	if ( model == NULL )
	{
		WARN( "Failed to open obj model (%s)!\n", sourcePath );
		return false;
	}

	unsigned int numVertices;
	float **vertices = ( float ** ) PlGetVectorArrayDataEx( model->vertices, &numVertices );
	if ( numVertices == 0 )
	{
		WARN( "Model has no vertices (%s)!\n", sourcePath );
		return false;
	}

	if ( model->numMaterials > 0 )
	{
		NdBranch *materialBranch = ndPushBackStringArray( root, "materials", NULL, 0 );
		for ( unsigned int i = 0; i < model->numMaterials; ++i )
		{
			char tmp[ 128 ];
			snprintf( tmp, sizeof( tmp ), "materials/%s.mat.n", model->materials[ i ].name );
			ndPushBackString( materialBranch, NULL, tmp );
		}
	}

	if ( model->numSubObjects > 0 )
	{
		NdBranch *meshArray = ndPushBackObjectArray( root, "meshes" );
		for ( unsigned int i = 0; i < model->numSubObjects; ++i )
		{
			NdBranch *mb = ndPushBackObject( meshArray, NULL );

			ndPushBackUI32( mb, "materialIndex", i );

			NdBranch *pb = ndPushBackF32Array( mb, "positions", NULL, 0 );
			NdBranch *nb = ndPushBackF32Array( mb, "normals", NULL, 0 );
			NdBranch *ub = ndPushBackF32Array( mb, "uvs", NULL, 0 );
			NdBranch *cb = ndPushBackF32Array( mb, "colours", NULL, 0 );

			unsigned int numFaces;
			ObjFace **faces = ( ObjFace ** ) PlGetVectorArrayDataEx( model->subObjects[ i ].faces, &numFaces );
			for ( unsigned int j = 0; j < numFaces; ++j )
			{
				unsigned int numTriangles = faces[ j ]->numEdges < 3 ? 0 : ( faces[ j ]->numEdges - 3 );
			}
		}
	}

	model_obj_destroy( model );

	return true;
}

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
	else if ( pl_strcasecmp( extension, "obj" ) == 0 && !serialize_obj( root, path ) )
		ERROR( "Failed to serialize obj model (%s)!\n", path )

	PlSetupPath( path, true, "%s/ship/models/%s." SS_APE_FORMAT_MODEL_EXTENSION, ss_com_project_get_local_path(), modelName );
	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
		ERROR( "Failed to write model: %s\n", ndGetErrorMessage() );
}
