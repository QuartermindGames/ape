// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_timer.h>

#include "modelconv.h"

#define VERSION "1.0.0"

enum
{
	CHANNEL_POSITION,
	CHANNEL_UV,
	CHANNEL_NORMAL,
	CHANNEL_COLOUR,
	MAX_CHANNELS
};

static void SerializeModelMesh( YNNodeBranch *root, const PLGMesh *mesh )
{
	NL_PushBackI32( root, "materialIndex", ( int32_t ) mesh->materialIndex );
	NL_PushBackI32( root, "numVertices", ( int32_t ) mesh->num_verts );

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
		YNNodeBranch *positionArray = YnNode_PushBackF32Array( root, "positions", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			NL_PushBackF32( positionArray, "x", mesh->vertices[ i ].position.x );
			NL_PushBackF32( positionArray, "y", mesh->vertices[ i ].position.y );
			NL_PushBackF32( positionArray, "z", mesh->vertices[ i ].position.z );
		}
	}
	if ( hasChannel[ CHANNEL_UV ] )
	{
		YNNodeBranch *uvArray = YnNode_PushBackF32Array( root, "uvs", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			NL_PushBackF32( uvArray, "x", mesh->vertices[ i ].st[ 0 ].x );
			NL_PushBackF32( uvArray, "y", mesh->vertices[ i ].st[ 0 ].y );
		}
	}
	if ( hasChannel[ CHANNEL_NORMAL ] )
	{
		YNNodeBranch *normalsArray = YnNode_PushBackF32Array( root, "normals", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			NL_PushBackF32( normalsArray, "x", mesh->vertices[ i ].normal.x );
			NL_PushBackF32( normalsArray, "y", mesh->vertices[ i ].normal.y );
			NL_PushBackF32( normalsArray, "z", mesh->vertices[ i ].normal.z );
		}
	}
	if ( hasChannel[ CHANNEL_COLOUR ] )
	{
		YNNodeBranch *coloursArray = YnNode_PushBackF32Array( root, "colours", NULL, 0 );
		for ( uint32_t i = 0; i < mesh->num_verts; ++i )
		{
			PLColourF32 colour = PlColourU8ToF32( &mesh->vertices[ i ].colour );
			NL_PushBackF32( coloursArray, "r", colour.r );
			NL_PushBackF32( coloursArray, "g", colour.g );
			NL_PushBackF32( coloursArray, "b", colour.b );
			NL_PushBackF32( coloursArray, "a", colour.a );
		}
	}

	NL_PushBackI32Array( root, "triangles", ( int32_t * ) mesh->indices, mesh->num_indices );
}

static YNNodeBranch *SerializeModel( const PLMModel *model )
{
	YNNodeBranch *root = NL_PushBackObj( NULL, "model" );
	NL_PushBackI8( root, "version", 2 );

	printf( "%u materials\n", model->numMaterials );
	YNNodeBranch *materialList = NL_PushBackStrArray( root, "materials", NULL, 0 );
	for ( uint32_t i = 0; i < model->numMaterials; ++i )
	{
		printf( " %u : %s\n", i, model->materials[ i ] );
		NL_PushBackStr( materialList, NULL, model->materials[ i ] );
	}

	printf( "%u meshes\n", model->numMeshes );
	YNNodeBranch *meshesList = YnNode_PushBackObjArray( root, "meshes" );
	for ( uint32_t i = 0; i < model->numMeshes; ++i )
	{
		printf( " %u : %s %u verts, %u tris\n", i,
		        model->materials[ model->meshes[ i ]->materialIndex ],
		        model->meshes[ i ]->num_verts,
		        model->meshes[ i ]->num_triangles );
		SerializeModelMesh( NL_PushBackObj( meshesList, "mesh" ), model->meshes[ i ] );
	}

	if ( model->type == PLM_MODELTYPE_SKELETAL )
	{
		NL_PushBackBool( root, "isAnimated", true );
		NL_PushBackI32( root, "rootBone", ( int32_t ) model->internal.skeletal_data.rootIndex );

		YNNodeBranch *bonesList = YnNode_PushBackObjArray( root, "bones" );
		for ( uint32_t i = 0; i < model->internal.skeletal_data.numBones; ++i )
		{
			YNNodeBranch *bone = NL_PushBackObj( bonesList, "bone" );
			NL_PushBackStr( bone, "name", model->internal.skeletal_data.bones[ i ].name );
			NL_PushBackI32( bone, "parent", ( int32_t ) model->internal.skeletal_data.bones[ i ].parent );
			YnNode_PushBackF32Array( bone, "position", ( float * ) &model->internal.skeletal_data.bones[ i ].position, 3 );
			YnNode_PushBackF32Array( bone, "orientation", ( float * ) &model->internal.skeletal_data.bones[ i ].orientation, 4 );
		}

#if 0// todo...
		NLNode *boneWeights = NL_PushBackObjArray( root, "boneWeights" );
		for ( uint32_t i = 0; i < model->internal.skeletal_data.numBoneWeights; ++i )
		{
			NLNode *boneWeight = NL_PushBackObj( boneWeights, "boneWeight" );
			NL_PushBackI32( boneWeight, "vertexIndex", ( int32_t ) model->internal.skeletal_data.weights[ i ].vertexIndex );
			NL_PushBackI32( boneWeight, "boneIndex", ( int32_t ) model->internal.skeletal_data.weights[ i ].boneIndex );
			NL_PushBackF32( boneWeight, "factor", model->internal.skeletal_data.weights[ i ].factor );
		}
#endif
	}
	else
	{
		NL_PushBackBool( root, "isAnimated", false );
	}

	return root;
}

int main( int argc, char **argv )
{
	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
	{
		fprintf( stderr, "Failed to initialize Hei library: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	printf( "modelconv v" VERSION " (" __DATE__ " " __TIME__ ")\n"
	        "Model conversion tooling for the Yin 3D Game Engine\n"
	        "Copyright (C) 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>\n"
	        "-------------------------------------------------------------------\n" );

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

	PLMModel *MDL_MD2_LoadFile( const char *path );
	PlmRegisterModelLoader( "md2", MDL_MD2_LoadFile );

	PLMModel *OC_MSH_LoadFile( const char *path );
	PlmRegisterModelLoader( "msh", OC_MSH_LoadFile );

	PLMModel *MDL_MDL_LoadFile( const char *path );
	PlmRegisterModelLoader( "mdl", MDL_MDL_LoadFile );

	double timeStart = PlGetCurrentSeconds();

	PLMModel *model = PlmLoadModel( modelPath );
	if ( model == NULL )
	{
		fprintf( stderr, "Failed to load model: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	YNNodeBranch *root = SerializeModel( model );
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

	YnNode_WriteFile( outPath, root, PlHasCommandLineArgument( "-r" ) ? NL_FILE_UTF8 : NL_FILE_BINARY );

	PlmDestroyModel( model );
	YnNode_DestroyBranch( root );

	double timeEnd = PlGetCurrentSeconds();

	printf( "-------------------------------------------------------------------\n"
	        "Finished conversion successfully in %.2lfs to \"%s\"\n",
	        timeEnd - timeStart, outPath );

	return EXIT_SUCCESS;
}
