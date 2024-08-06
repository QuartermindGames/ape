// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "cook.h"
#include "model/model_obj.h"
#include "yin/core_world.h"

static void process_properties( const char *worldName, AcmBranch *root )
{
	PLPath path;
	PlSetupPath( path, true, "worlds/%s/%s." APE_WORLD_EXTENSION_CFG, worldName, worldName );
	AcmBranch *properties = acm_load_file( path, "properties" );
	if ( properties == NULL )
	{
		WARN( "Failed to open world properties file (%s): %s\n", path, acm_get_error_message() );
		return;
	}
	acm_branch_push_back_branch( root, properties );
	acm_branch_destroy( properties );
}

static void process_geometry( const char *worldName, AcmBranch *root )
{
	PLPath path;
	PlSetupPath( path, true, "worlds/%s/%s.obj", worldName, worldName );
	ObjModel *model = model_obj_load( path );
	if ( model == NULL )
	{
		ERROR( "Failed to open OBJ model (%s)!\n", path );
	}

	root = acm_branch_push_back_object( root, "geometry" );

	AcmBranch *child;
	if ( model->numMaterials > 0 )
	{
		printf( "Building material table (%u)...\n", model->numMaterials );
		child = acm_branch_push_back_string_array( root, "materials", NULL, 0 );
		for ( unsigned int i = 0; i < model->numMaterials; ++i )
		{
			char tmp[ 128 ];
			snprintf( tmp, sizeof( tmp ), "materials/%s.mat.n", model->materials[ i ].name );
			if ( !PlFileExists( tmp ) )
			{
				snprintf( tmp, sizeof( tmp ), "textures/%s.mat.n", model->materials[ i ].name );
				if ( !PlFileExists( tmp ) )
				{
					WARN( "Failed to find material (%s)!\n", tmp );
				}
			}

			printf( " %u \"%s\"\n", acm_branch_get_num_of_children( child ), tmp );
			acm_branch_push_back_string( child, NULL, tmp );
		}
	}
	printf( "%u materials\n", acm_branch_get_num_of_children( child ) );

	if ( model->numSubObjects > 0 )
	{
		child = acm_branch_push_back_object_array( root, "rooms" );
		for ( unsigned int i = 0; i < model->numSubObjects; ++i )
		{
			// Ignore sub objects that don't have any faces
			unsigned int numFaces = PlGetNumVectorArrayElements( model->subObjects[ i ].faces );
			if ( numFaces == 0 )
			{
				continue;
			}

			AcmBranch *roomBranch = acm_branch_push_back_object( child, NULL );
			acm_branch_push_back_int32( roomBranch, "uid", i );
			acm_branch_push_back_string( roomBranch, "tag", model->subObjects[ i ].name );
			acm_branch_push_back_float32_array( roomBranch, "mins", ( float * ) &model->subObjects[ i ].mins, 3 );
			acm_branch_push_back_float32_array( roomBranch, "maxs", ( float * ) &model->subObjects[ i ].maxs, 3 );
		}

		acm_branch_push_back_bool( root, "hasColour", model->storesColour );
		child = acm_branch_push_back_float32_array( root, "vertices", NULL, 0 );
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( model->vertices ); ++j )
		{
			ObjVertex *v = PlGetVectorArrayElementAt( model->vertices, j );
			if ( v == NULL )
			{
				ERROR( "Attempted to retrieve an invalid vertex (%u): %s\n", j, PlGetError() );
			}

			acm_branch_push_back_float32( child, NULL, v->position.x );
			acm_branch_push_back_float32( child, NULL, v->position.y );
			acm_branch_push_back_float32( child, NULL, v->position.z );
			if ( model->storesColour )
			{
				acm_branch_push_back_float32( child, NULL, v->colour.x );
				acm_branch_push_back_float32( child, NULL, v->colour.y );
				acm_branch_push_back_float32( child, NULL, v->colour.z );
			}
		}

#if 0
		child = ndPushBackF32Array( root, "normals", NULL, 0 );
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( model->normals ); ++j )
		{
			PLVector3 *v = PlGetVectorArrayElementAt( model->normals, j );
			assert( v != NULL );
			if ( v == NULL )
				ERROR( "Attempted to retrieve an invalid normal (%u): %s\n", j, PlGetError() );

			ndPushBackF32( child, NULL, v->x );
			ndPushBackF32( child, NULL, v->y );
			ndPushBackF32( child, NULL, v->z );
		}

		child = ndPushBackF32Array( root, "uvs", NULL, 0 );
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( model->textureCoords ); ++j )
		{
			PLVector2 *v = PlGetVectorArrayElementAt( model->textureCoords, j );
			assert( v != NULL );
			if ( v == NULL )
				ERROR( "Attempted to retrieve an invalid normal (%u): %s\n", j, PlGetError() );

			ndPushBackF32( child, NULL, v->x );
			ndPushBackF32( child, NULL, v->y );
		}
#endif

		child = acm_branch_push_back_object_array( root, "faces" );
		for ( unsigned int i = 0; i < model->numSubObjects; ++i )
		{
			unsigned int numFaces;
			ObjFace    **faces = ( ObjFace    **) PlGetVectorArrayDataEx( model->subObjects[ i ].faces, &numFaces );
			for ( unsigned int j = 0; j < numFaces; ++j )
			{
				if ( strncmp( model->materials[ faces[ j ]->material ].name, "tools/skip", 10 ) == 0 )
				{
					continue;
				}

				AcmBranch *faceBranch = acm_branch_push_back_object( child, NULL );
				acm_branch_push_back_float32_array( faceBranch, "normal", ( float * ) &faces[ j ]->normal, 3 );
				acm_branch_push_back_uint32( faceBranch, "material", faces[ j ]->material );
				acm_branch_push_back_uint32( faceBranch, "roomIndex", i );

				AcmBranch *verticesBranch = acm_branch_push_back_object_array( faceBranch, "edges" );
				for ( unsigned int k = 0; k < faces[ j ]->numEdges; ++k )
				{
					AcmBranch *edgeBranch = acm_branch_push_back_object( verticesBranch, NULL );
					acm_branch_push_back_uint32( edgeBranch, "vertexIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_VERTEX ] );
					//ndPushBackUI32( edgeBranch, "normalIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_NORMAL ] );
					//ndPushBackUI32( edgeBranch, "uvIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_TEXTURE ] );

					// For now, because we've already got the deserialiser written out like it, let's bundle them as explicit values,
					// but in the longer term we should probably consider the above instead

					PLVector3 *normal = PlGetVectorArrayElementAt( model->normals, faces[ j ]->indices[ k ][ OBJ_INDEX_NORMAL ] );
					if ( normal != NULL )
					{
						acm_branch_push_back_float32_array( edgeBranch, "normal", ( float * ) normal, 3 );
					}
					PLVector2 *uv = PlGetVectorArrayElementAt( model->textureCoords, faces[ j ]->indices[ k ][ OBJ_INDEX_TEXTURE ] );
					if ( uv != NULL )
					{
						acm_branch_push_back_float32_array( edgeBranch, "uv", ( float * ) &( PLVector3 ){ uv->x, -uv->y }, 2 );
					}
				}
			}
		}
	}
	printf( "%u sub objects\n", model->numSubObjects );

	model_obj_destroy( model );
}

void cook_world_process( const char *worldName )
{
	printf( "Processing world: %s\n", worldName );

	AcmBranch *root = acm_branch_push_back_object( NULL, "world" );
	acm_branch_push_back_uint32( root, "version", APE_WORLD_VERSION );

	process_properties( worldName, root );
	process_geometry( worldName, root );

	PLPath path;
	PlSetupPath( path, true, "%s/ship/worlds/%s." APE_WORLD_EXTENSION, com_project_get_local_path(), worldName );
	if ( !acm_write_file( path, root, ND_FILE_BINARY ) )
	{
		ERROR( "Failed to write world: %s\n", acm_get_error_message() );
	}
}
