// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_timer.h>
#include <plmodel/plm.h>

#include "ape_private.h"

#include "yin/node.h"

#include "importer_model.h"

enum
{
	CHANNEL_POSITION,
	CHANNEL_UV,
	CHANNEL_NORMAL,
	CHANNEL_COLOUR,
	MAX_CHANNELS
};

static void SerializeModelMesh( NdBranch *root, const PLGMesh *mesh )
{
	ndPushBackI32( root, "materialIndex", ( int32_t ) mesh->materialIndex );
	ndPushBackI32( root, "numVertices", ( int32_t ) mesh->num_verts );

	bool hasChannel[ MAX_CHANNELS ];
	PL_ZERO( hasChannel, sizeof( bool ) * MAX_CHANNELS );
	for ( uint32_t i = 0; i < mesh->num_verts; ++i )
	{
		if ( !PlCompareVector3( &mesh->vertices[ i ].position, &pl_vecOrigin3 ) )
		{
			hasChannel[ CHANNEL_POSITION ] = true;
		}
		if ( !PlCompareVector2( &mesh->vertices[ i ].st[ 0 ], &pl_vecOrigin2 ) )
		{
			hasChannel[ CHANNEL_UV ] = true;
		}
		if ( !PlCompareVector3( &mesh->vertices[ i ].normal, &pl_vecOrigin3 ) )
		{
			hasChannel[ CHANNEL_NORMAL ] = true;
		}
		if ( !PlCompareColour( mesh->vertices[ i ].colour, PL_COLOURU8( 255, 255, 255, 255 ) ) )
		{
			hasChannel[ CHANNEL_COLOUR ] = true;
		}

		// if all channels are enabled, just jump out early
		unsigned int j;
		for ( j = 0; j < MAX_CHANNELS; ++j )
		{
			if ( !hasChannel[ j ] )
			{
				continue;
			}

			break;
		}
		if ( j >= MAX_CHANNELS )
		{
			break;
		}
	}

	if ( hasChannel[ CHANNEL_POSITION ] )
	{
		NdBranch *positionArray = ndPushBackF32Array( root, "positions", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			ndPushBackF32( positionArray, "x", mesh->vertices[ i ].position.x );
			ndPushBackF32( positionArray, "y", mesh->vertices[ i ].position.y );
			ndPushBackF32( positionArray, "z", mesh->vertices[ i ].position.z );
		}
	}
	if ( hasChannel[ CHANNEL_UV ] )
	{
		NdBranch *uvArray = ndPushBackF32Array( root, "uvs", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			ndPushBackF32( uvArray, "x", mesh->vertices[ i ].st[ 0 ].x );
			ndPushBackF32( uvArray, "y", mesh->vertices[ i ].st[ 0 ].y );
		}
	}
	if ( hasChannel[ CHANNEL_NORMAL ] )
	{
		NdBranch *normalsArray = ndPushBackF32Array( root, "normals", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			ndPushBackF32( normalsArray, "x", mesh->vertices[ i ].normal.x );
			ndPushBackF32( normalsArray, "y", mesh->vertices[ i ].normal.y );
			ndPushBackF32( normalsArray, "z", mesh->vertices[ i ].normal.z );
		}
	}
	if ( hasChannel[ CHANNEL_COLOUR ] )
	{
		NdBranch *coloursArray = ndPushBackF32Array( root, "colours", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			PLColourF32 colour = PlColourU8ToF32( &mesh->vertices[ i ].colour );
			ndPushBackF32( coloursArray, "r", colour.r );
			ndPushBackF32( coloursArray, "g", colour.g );
			ndPushBackF32( coloursArray, "b", colour.b );
			ndPushBackF32( coloursArray, "a", colour.a );
		}
	}

	ndPushBackI32Array( root, "triangles", ( int32_t * ) mesh->indices, mesh->num_indices );
}

static NdBranch *SerializeModel( const PLMModel *model )
{
	NdBranch *root = ndPushBackObject( NULL, "model" );
	ndPushBackI8( root, "version", 2 );

	printf( "%u materials\n", model->numMaterials );
	NdBranch *materialList = ndPushBackStringArray( root, "materials", NULL, 0 );
	for ( uint32_t i = 0; i < model->numMaterials; ++i )
	{
		printf( " %u : %s\n", i, model->materials[ i ] );
		ndPushBackString( materialList, NULL, model->materials[ i ] );
	}

	printf( "%u meshes\n", model->numMeshes );
	NdBranch *meshesList = ndPushBackObjectArray( root, "meshes" );
	for ( uint32_t i = 0; i < model->numMeshes; ++i )
	{
		printf( " %u : %s %u verts, %u tris\n", i,
		        model->materials[ model->meshes[ i ]->materialIndex ],
		        model->meshes[ i ]->num_verts,
		        model->meshes[ i ]->num_triangles );
		SerializeModelMesh( ndPushBackObject( meshesList, "mesh" ), model->meshes[ i ] );
	}

	if ( model->type == PLM_MODELTYPE_SKELETAL )
	{
		ndPushBackBool( root, "isAnimated", true );
		ndPushBackI32( root, "rootBone", ( int32_t ) model->internal.skeletal_data.rootIndex );

		NdBranch *bonesList = ndPushBackObjectArray( root, "bones" );
		for ( uint32_t i = 0; i < model->internal.skeletal_data.numBones; ++i )
		{
			NdBranch *bone = ndPushBackObject( bonesList, "bone" );
			ndPushBackString( bone, "name", model->internal.skeletal_data.bones[ i ].name );
			ndPushBackI32( bone, "parent", ( int32_t ) model->internal.skeletal_data.bones[ i ].parent );
			ndPushBackF32Array( bone, "position", ( float * ) &model->internal.skeletal_data.bones[ i ].position, 3 );
			ndPushBackF32Array( bone, "orientation", ( float * ) &model->internal.skeletal_data.bones[ i ].orientation, 4 );
		}

#if 0// todo...
		NLNode *boneWeights = ndPushBackObjArray( root, "boneWeights" );
		for ( uint32_t i = 0; i < model->internal.skeletal_data.numBoneWeights; ++i )
		{
			NLNode *boneWeight = ndPushBackObj( boneWeights, "boneWeight" );
			ndPushBackI32( boneWeight, "vertexIndex", ( int32_t ) model->internal.skeletal_data.weights[ i ].vertexIndex );
			ndPushBackI32( boneWeight, "boneIndex", ( int32_t ) model->internal.skeletal_data.weights[ i ].boneIndex );
			ndPushBackF32( boneWeight, "factor", model->internal.skeletal_data.weights[ i ].factor );
		}
#endif
	}
	else
	{
		ndPushBackBool( root, "isAnimated", false );
	}

	return root;
}

#if 0
int main( int argc, char **argv )
{
	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
	{
		fprintf( stderr, "Failed to initialize Hei library: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	const char *modelPath = PlGetCommandLineArgumentValue( "-in" );
	if ( modelPath == NULL )
	{
		printf( "Usage: modelconv -in <model-path> [options]\n"
		        "Options:\n"
		        "   -in  = Input location (required).\n"
		        "	-out = Output location.\n"
		        "	-r   = Export to human-readable format.\n" );
		return EXIT_SUCCESS;
	}

	double timeStart = PlGetCurrentSeconds();

	PLMModel *model = PlmLoadModel( modelPath );
	if ( model == NULL )
	{
		fprintf( stderr, "Failed to load model: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	NdBranch *root = SerializeModel( model );
	if ( root == NULL )
	{
		fprintf( stderr, "Failed to serialize model!\n" );
		return EXIT_FAILURE;
	}

	const char *outPath = PlGetCommandLineArgumentValue( "-out" );
	if ( outPath == NULL )
	{
		outPath = "./dump.mdl.n";
	}

	ndWriteFile( outPath, root, PlHasCommandLineArgument( "-r" ) ? ND_FILE_UTF8 : ND_FILE_BINARY );

	PlmDestroyModel( model );
	ndDestroyBranch( root );

	double timeEnd = PlGetCurrentSeconds();

	printf( "-------------------------------------------------------------------\n"
	        "Finished conversion successfully in %.2lfs to \"%s\"\n",
	        timeEnd - timeStart, outPath );

	return EXIT_SUCCESS;
}
#endif
