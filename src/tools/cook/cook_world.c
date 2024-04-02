// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "cook.h"
#include "model/model_obj.h"
#include "yin/core_world.h"

static void process_properties( const char *worldName, NdBranch *root )
{
	PLPath path;
	PlSetupPath( path, true, "worlds/%s/%s." APE_WORLD_EXTENSION_CFG, worldName, worldName );
	NdBranch *properties = nd_load_file( path, "properties" );
	if ( properties == NULL )
	{
		ERROR( "Failed to open world properties file (%s): %s\n", path, nd_get_error_message() );
	}
	nd_branch_push_back_branch( root, properties );
	nd_branch_destroy( properties );
}

static void process_geometry( const char *worldName, NdBranch *root )
{
	PLPath path;
	PlSetupPath( path, true, "worlds/%s/%s.obj", worldName, worldName );
	ObjModel *model = model_obj_load( path );
	if ( model == NULL )
	{
		ERROR( "Failed to open OBJ model (%s)!\n", path );
	}

	root = nd_branch_push_back_object( root, "geometry" );

	NdBranch *child;
	if ( model->numMaterials > 0 )
	{
		child = nd_branch_push_back_string_array( root, "materials", NULL, 0 );
		for ( unsigned int i = 0; i < model->numMaterials; ++i )
		{
			char tmp[ 128 ];
			snprintf( tmp, sizeof( tmp ), "materials/world/%s.mat.n", model->materials[ i ].name );
			nd_branch_push_back_string( child, NULL, tmp );
		}
	}
	printf( "%u materials\n", model->numMaterials );

	if ( model->numSubObjects > 0 )
	{
		child = nd_branch_push_back_object_array( root, "rooms" );
		for ( unsigned int i = 0; i < model->numSubObjects; ++i )
		{
			// Ignore sub objects that don't have any faces
			unsigned int numFaces = PlGetNumVectorArrayElements( model->subObjects[ i ].faces );
			if ( numFaces == 0 )
				continue;

			NdBranch *roomBranch = nd_branch_push_back_object( child, NULL );
			nd_branch_push_back_int32( roomBranch, "uid", i );
			nd_branch_push_back_string( roomBranch, "tag", model->subObjects[ i ].name );
			nd_branch_push_back_float32_array( roomBranch, "mins", ( float * ) &model->subObjects[ i ].mins, 3 );
			nd_branch_push_back_float32_array( roomBranch, "maxs", ( float * ) &model->subObjects[ i ].maxs, 3 );
		}

		child = nd_branch_push_back_float32_array( root, "vertices", NULL, 0 );
		for ( unsigned int j = 0; j < PlGetNumVectorArrayElements( model->vertices ); ++j )
		{
			PLVector3 *v = PlGetVectorArrayElementAt( model->vertices, j );
			if ( v == NULL )
			{
				ERROR( "Attempted to retrieve an invalid vertex (%u): %s\n", j, PlGetError() );
			}

			nd_branch_push_back_float32( child, NULL, v->x );
			nd_branch_push_back_float32( child, NULL, v->y );
			nd_branch_push_back_float32( child, NULL, v->z );
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

		child = nd_branch_push_back_object_array( root, "faces" );
		for ( unsigned int i = 0; i < model->numSubObjects; ++i )
		{
			unsigned int numFaces;
			ObjFace **faces = ( ObjFace ** ) PlGetVectorArrayDataEx( model->subObjects[ i ].faces, &numFaces );
			for ( unsigned int j = 0; j < numFaces; ++j )
			{
				NdBranch *faceBranch = nd_branch_push_back_object( child, NULL );
				nd_branch_push_back_float32_array( faceBranch, "normal", ( float * ) &faces[ j ]->normal, 3 );
				nd_branch_push_back_uint32( faceBranch, "material", faces[ j ]->material );
				nd_branch_push_back_int32( faceBranch, "smoothingGroup", faces[ j ]->smoothingGroup );
				nd_branch_push_back_uint32( faceBranch, "roomIndex", i );

				NdBranch *verticesBranch = nd_branch_push_back_object_array( faceBranch, "edges" );
				for ( unsigned int k = 0; k < faces[ j ]->numEdges; ++k )
				{
					NdBranch *edgeBranch = nd_branch_push_back_object( verticesBranch, NULL );
					nd_branch_push_back_uint32( edgeBranch, "vertexIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_VERTEX ] );
					//ndPushBackUI32( edgeBranch, "normalIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_NORMAL ] );
					//ndPushBackUI32( edgeBranch, "uvIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_TEXTURE ] );

					// For now, because we've already got the deserialiser written out like it, let's bundle them as explicit values,
					// but in the longer term we should probably consider the above instead

					PLVector3 *normal = PlGetVectorArrayElementAt( model->normals, faces[ j ]->indices[ k ][ OBJ_INDEX_NORMAL ] );
					if ( normal != NULL )
					{
						nd_branch_push_back_float32_array( edgeBranch, "normal", ( float * ) normal, 3 );
					}
					PLVector2 *uv = PlGetVectorArrayElementAt( model->textureCoords, faces[ j ]->indices[ k ][ OBJ_INDEX_TEXTURE ] );
					if ( uv != NULL )
					{
						nd_branch_push_back_float32_array( edgeBranch, "uv", ( float * ) &( PLVector3 ){ uv->x, -uv->y }, 2 );
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

	NdBranch *root = nd_branch_push_back_object( NULL, "world" );
	nd_branch_push_back_uint32( root, "version", APE_WORLD_VERSION );

	process_properties( worldName, root );
	process_geometry( worldName, root );

	PLPath path;
	PlSetupPath( path, true, "%s/ship/worlds/%s." APE_WORLD_EXTENSION, com_project_get_local_path(), worldName );
	if ( !nd_write_file( path, root, ND_FILE_BINARY ) )
	{
		ERROR( "Failed to write world: %s\n", nd_get_error_message() );
	}
}
