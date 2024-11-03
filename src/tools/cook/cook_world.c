// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "cook.h"
#include "model/model_obj.h"
#include "yin/core_world.h"

static void import_obj_geometry( const char *path, const char *worldName )
{
	ObjModel *model = model_obj_load( path );
	if ( model == nullptr )
	{
		ERROR( "Failed to open OBJ model (%s)!\n", path );
	}

	for ( uint i = 0; i < model->numSubObjects; ++i )
	{
		AcmBranch *child;
		AcmBranch *root = acm_branch_push_back_object( nullptr, "brush" );

		acm_branch_push_back_bool( root, "hasColour", model->storesColour );
		child = acm_push_array_f32( root, "vertices", nullptr, 0 );
		for ( uint j = 0; j < PlGetNumVectorArrayElements( model->vertices ); ++j )
		{
			ObjVertex *v = PlGetVectorArrayElementAt( model->vertices, j );
			if ( v == NULL )
			{
				ERROR( "Attempted to retrieve an invalid vertex (%u): %s\n", j, PlGetError() );
			}

			acm_branch_push_back_float32( child, nullptr, v->position.x );
			acm_branch_push_back_float32( child, nullptr, v->position.y );
			acm_branch_push_back_float32( child, nullptr, v->position.z );
			if ( model->storesColour )
			{
				acm_branch_push_back_float32( child, nullptr, v->colour.x );
				acm_branch_push_back_float32( child, nullptr, v->colour.y );
				acm_branch_push_back_float32( child, nullptr, v->colour.z );
			}
		}

		child = acm_branch_push_back_object_array( root, "faces" );
		uint      numFaces;
		ObjFace **faces = ( ObjFace ** ) PlGetVectorArrayDataEx( model->subObjects[ i ].faces, &numFaces );
		for ( uint j = 0; j < numFaces; ++j )
		{
			AcmBranch *faceBranch = acm_branch_push_back_object( child, nullptr );
			acm_push_array_f32( faceBranch, "normal", ( float * ) &faces[ j ]->normal, 3 );
			acm_push_string( faceBranch, "material", model->materials[ faces[ j ]->material ].name, false );

#if 0
			// determine the validity of the face
			PLVector3 r;
			for ( uint k = 0; k < faces[ j ]->numEdges; ++k )
			{
				ObjVertex *va = PlGetVectorArrayElementAt( model->vertices, ( k + 1 ) % faces[ j ]->numEdges );
				ObjVertex *vb = PlGetVectorArrayElementAt( model->vertices, k );
				ObjVertex *vc = PlGetVectorArrayElementAt( model->vertices, ( k + 2 ) % faces[ j ]->numEdges );

				PLVector3 n = PlVector3CrossProduct( PlSubtractVector3( va->position, vb->position ),
				                                     PlSubtractVector3( vb->position, vc->position ) );
				if ( k == 0 )
				{
					r = n;
					continue;
				}

				if ( PlVector3DotProduct( r, n ) < 0.f )
				{
					WARN( "Concave face detected, this will result in dumb shit!\n" );
					//TODO: split...
				}
			}
#endif

			AcmBranch *verticesBranch = acm_branch_push_back_object_array( faceBranch, "edges" );
			for ( uint k = 0; k < faces[ j ]->numEdges; ++k )
			{
				AcmBranch *edgeBranch = acm_branch_push_back_object( verticesBranch, nullptr );
				acm_push_uint32( edgeBranch, "vertexIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_VERTEX ] );

				PLVector3 *normal = PlGetVectorArrayElementAt( model->normals, faces[ j ]->indices[ k ][ OBJ_INDEX_NORMAL ] );
				if ( normal != NULL )
				{
					acm_push_array_f32( edgeBranch, "normal", ( float * ) normal, 3 );
				}
				PLVector2 *uv = PlGetVectorArrayElementAt( model->textureCoords, faces[ j ]->indices[ k ][ OBJ_INDEX_TEXTURE ] );
				if ( uv != NULL )
				{
					acm_push_array_f32( edgeBranch, "uv", ( float * ) &( PLVector3 ){ uv->x, -uv->y }, 2 );
				}
			}
		}

		PLPath path;
		PlSetupPath( path, true, "%s/ship/worlds/%s/brushes", com_project_get_local_path(), worldName );
		if ( !PlCreatePath( path ) )
		{
			ERROR( "Failed to create output path (%s): %s\n", path, PlGetError() );
		}

		PlAppendPathEx( path, true, "/%s." APE_WORLD_BRUSH_EXTENSION, model->subObjects[ i ].name );
		if ( !acm_write_file( path, root, ND_FILE_BINARY ) )
		{
			ERROR( "Failed to write brush (%s): %s\n", path, acm_get_error_message() );
		}
	}
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

#if defined( APE_USE_NEW_WORLD_LAYOUT )
	import_obj_geometry( path, worldName );
#endif

	root = acm_branch_push_back_object( root, "geometry" );

	AcmBranch *child;
	if ( model->numMaterials > 0 )
	{
		printf( "Building material table (%u)...\n", model->numMaterials );
		child = acm_branch_push_back_string_array( root, "materials", nullptr, 0 );
		for ( uint i = 0; i < model->numMaterials; ++i )
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
			acm_push_string( child, nullptr, tmp, false );
		}
	}
	printf( "%u materials\n", acm_branch_get_num_of_children( child ) );

	if ( model->numSubObjects > 0 )
	{
		// determine the max extent of the room with all the sub objects into account
		PLVector3 mins = model->subObjects[ 0 ].mins;
		PLVector3 maxs = model->subObjects[ 0 ].maxs;
		for ( uint i = 1; i < model->numSubObjects; ++i )
		{
			if ( mins.x < model->subObjects[ i ].mins.x ) mins.x = model->subObjects[ i ].mins.x;
			if ( mins.y < model->subObjects[ i ].mins.y ) mins.y = model->subObjects[ i ].mins.y;
			if ( mins.z < model->subObjects[ i ].mins.z ) mins.z = model->subObjects[ i ].mins.z;
			if ( maxs.x > model->subObjects[ i ].maxs.x ) maxs.x = model->subObjects[ i ].maxs.x;
			if ( maxs.y > model->subObjects[ i ].maxs.y ) maxs.y = model->subObjects[ i ].maxs.y;
			if ( maxs.z > model->subObjects[ i ].maxs.z ) maxs.z = model->subObjects[ i ].maxs.z;
		}

		acm_branch_push_back_bool( root, "hasColour", model->storesColour );
		child = acm_push_array_f32( root, "vertices", nullptr, 0 );
		for ( uint j = 0; j < PlGetNumVectorArrayElements( model->vertices ); ++j )
		{
			ObjVertex *v = PlGetVectorArrayElementAt( model->vertices, j );
			if ( v == NULL )
			{
				ERROR( "Attempted to retrieve an invalid vertex (%u): %s\n", j, PlGetError() );
			}

			acm_branch_push_back_float32( child, nullptr, v->position.x );
			acm_branch_push_back_float32( child, nullptr, v->position.y );
			acm_branch_push_back_float32( child, nullptr, v->position.z );
			if ( model->storesColour )
			{
				acm_branch_push_back_float32( child, nullptr, v->colour.x );
				acm_branch_push_back_float32( child, nullptr, v->colour.y );
				acm_branch_push_back_float32( child, nullptr, v->colour.z );
			}
		}

		child = acm_branch_push_back_object_array( root, "faces" );
		for ( uint i = 0; i < model->numSubObjects; ++i )
		{
			uint      numFaces;
			ObjFace **faces = ( ObjFace ** ) PlGetVectorArrayDataEx( model->subObjects[ i ].faces, &numFaces );
			for ( uint j = 0; j < numFaces; ++j )
			{
				if ( strncmp( model->materials[ faces[ j ]->material ].name, "tools/skip", 10 ) == 0 )
				{
					continue;
				}

				AcmBranch *faceBranch = acm_branch_push_back_object( child, nullptr );
				acm_push_array_f32( faceBranch, "normal", ( float * ) &faces[ j ]->normal, 3 );
				acm_push_uint32( faceBranch, "material", faces[ j ]->material );
				acm_push_uint32( faceBranch, "roomIndex", 0 );

				AcmBranch *verticesBranch = acm_branch_push_back_object_array( faceBranch, "edges" );
				for ( uint k = 0; k < faces[ j ]->numEdges; ++k )
				{
					AcmBranch *edgeBranch = acm_branch_push_back_object( verticesBranch, nullptr );
					acm_push_uint32( edgeBranch, "vertexIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_VERTEX ] );
					//ndPushBackUI32( edgeBranch, "normalIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_NORMAL ] );
					//ndPushBackUI32( edgeBranch, "uvIndex", faces[ j ]->indices[ k ][ OBJ_INDEX_TEXTURE ] );

					// For now, because we've already got the deserialiser written out like it, let's bundle them as explicit values,
					// but in the longer term we should probably consider the above instead

					PLVector3 *normal = PlGetVectorArrayElementAt( model->normals, faces[ j ]->indices[ k ][ OBJ_INDEX_NORMAL ] );
					if ( normal != NULL )
					{
						acm_push_array_f32( edgeBranch, "normal", ( float * ) normal, 3 );
					}
					PLVector2 *uv = PlGetVectorArrayElementAt( model->textureCoords, faces[ j ]->indices[ k ][ OBJ_INDEX_TEXTURE ] );
					if ( uv != NULL )
					{
						acm_push_array_f32( edgeBranch, "uv", ( float * ) &( PLVector3 ){ uv->x, -uv->y }, 2 );
					}
				}
			}
		}
	}
	printf( "%u sub objects\n", model->numSubObjects );

	{
		AcmBranch *roomBranch = acm_branch_push_back_object( acm_branch_push_back_object_array( root, "rooms" ), nullptr );
		acm_push_uint32( roomBranch, "version", APE_WORLD_ROOM_VERSION );
		acm_push_string( roomBranch, "name", worldName, false );
		acm_branch_push_back_int32( roomBranch, "uid", 0 );

#if defined( APE_USE_NEW_WORLD_LAYOUT )
		AcmBranch *nodesBranch = acm_branch_push_back_object_array( roomBranch, "nodes" );
		for ( unsigned int i = 0; i < model->numSubObjects; ++i )
		{
			PlSetupPath( path, true, "worlds/%s/brushes/%s." APE_WORLD_BRUSH_EXTENSION, worldName, model->subObjects[ i ].name );
			if ( !PlCreatePath( path ) )
			{
				ERROR( "Failed to create output path (%s): %s\n", path, PlGetError() );
			}

			AcmBranch *nodeBranch = acm_branch_push_back_object( nodesBranch, "node" );
			acm_push_string( nodeBranch, "class", "brush", false );
			acm_push_string( nodeBranch, "path", path, false );
		}

		PlSetupPath( path, true, "%s/ship/worlds/%s/rooms", com_project_get_local_path(), worldName );
		if ( !PlCreatePath( path ) )
		{
			ERROR( "Failed to create output path (%s): %s\n", path, PlGetError() );
		}

		PlAppendPathEx( path, true, "/%s." APE_WORLD_ROOM_EXTENSION, worldName );
		if ( !acm_write_file( path, roomBranch, ND_FILE_BINARY ) )
		{
			ERROR( "Failed to write room (%s): %s\n", path, acm_get_error_message() );
		}

		PlSetupPath( path, true, "worlds/%s/rooms/%s." APE_WORLD_ROOM_EXTENSION, worldName, worldName );
		acm_push_string( root, "startRoom", path, false );
#endif
	}

	model_obj_destroy( model );
}

void cook_world_process( const char *worldName )
{
	printf( "Processing world: %s\n", worldName );

	AcmBranch *root = acm_branch_push_back_object( nullptr, "world" );
	acm_push_uint32( root, "version", APE_WORLD_VERSION );

	process_geometry( worldName, root );

	PLPath path;
	PlSetupPath( path, true, "%s/ship/worlds/%s/%s." APE_WORLD_EXTENSION, com_project_get_local_path(), worldName, worldName );
	if ( !acm_write_file( path, root, ND_FILE_BINARY ) )
	{
		ERROR( "Failed to write world: %s\n", acm_get_error_message() );
	}
}
