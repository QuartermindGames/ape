// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: IMF loader and serialisation.
//			This is our *core* format; everything is ideally converted into this.
// Author:  Mark E. Sowden

#include "plcore/pl_filesystem.h"

#include "qmos/public/qm_os_memory.h"

#include "acm/acm.h"

#include "io_model/public/io_model.h"

static IOModel *imf_parse( IOModel *model, AcmBranch *root, IOModelResult *result )
{
	const uint16_t version = acm_get_uint( root, "version", UINT16_MAX );
	if ( version == UINT16_MAX || version > IO_MODEL_IMF_VERSION )
	{
		IO_MODEL_RESULT( result, "invalid version", IO_MODEL_RESULT_CODE_VERSION_ERROR );
		return nullptr;
	}

	AcmBranch   *branch;
	unsigned int numFloatElements;
	if ( ( branch = acm_get_child_by_name( root, "vertexFormatDescriptor" ) ) != nullptr )
	{
		numFloatElements = acm_get_uint( branch, "numFloatElements", 0 );
		if ( numFloatElements == 0 )
		{
			IO_MODEL_RESULT( result, "invalid number of float elements per vertex descriptor", IO_MODEL_RESULT_CODE_UNSUPPORTED_ERROR );
			return nullptr;
		}
	}
	else
	{
		IO_MODEL_RESULT( result, "model has no vertex descriptor", IO_MODEL_RESULT_CODE_UNSUPPORTED_ERROR );
		return nullptr;
	}

	AcmBranch *meshArray = acm_get_child_by_name( root, "meshes" );
	if ( meshArray == NULL || ( model->numMaterials = acm_get_num_of_children( meshArray ) ) == 0 )
	{
		IO_MODEL_RESULT( result, "no meshes for model", IO_MODEL_RESULT_CODE_UNSUPPORTED_ERROR );
		return nullptr;
	}
	if ( model->numMaterials >= IO_MODEL_MAX_MATERIALS )
	{
		//ape_console_warning_( "Unexpected number of meshes (%u >= %u)!\n", model->numMaterials, IO_MODEL_MAX_MATERIALS );
		model->numMaterials = IO_MODEL_MAX_MATERIALS - 1;
	}

	float       *vertices    = nullptr;
	unsigned int numVertices = 0;
	if ( ( branch = acm_get_child_by_name( root, "vertices" ) ) != nullptr )
	{
		unsigned int numIndices = acm_get_num_of_children( branch );
		if ( numIndices >= 3 )
		{
			vertices    = QM_OS_MEMORY_NEW_( float, numIndices );
			numVertices = numIndices / numFloatElements;
			acm_branch_get_float32_array( branch, vertices, numIndices );
		}
		else
		{
			//ape_console_warning_( "Invalid number (%u) of positions in model!\n", numVertices );
			numVertices = 0;
		}
	}

	if ( numVertices == 0 )
	{
		IO_MODEL_RESULT( result, "mesh has no vertices", IO_MODEL_RESULT_CODE_UNSUPPORTED_ERROR );
		return nullptr;
	}

	model->numVertices = numVertices;
	model->vertices    = QM_OS_MEMORY_NEW_( IOModelVertex, model->numVertices );
	const float *v     = vertices;
	for ( unsigned int i = 0; i < numVertices; ++i, v += numFloatElements )
	{
		model->vertices[ i ].position = *( const QmMathVector3f * ) &v[ 0 ];
		model->vertices[ i ].normal   = *( const QmMathVector3f * ) &v[ 3 ];
		model->vertices[ i ].uv       = *( const QmMathVector2f * ) &v[ 6 ];
	}

	return model;
}

IOModel *io_model_imf_load_( IOModel *model, QmFsFile *file, IOModelResult *result )
{
	if ( PlCacheFile( file ) == nullptr )
	{
		IO_MODEL_RESULT( result, "failed to cache file", IO_MODEL_RESULT_CODE_IO_ERROR );
		return nullptr;
	}

	const void  *ptr  = qm_fs_file_get_data( file );
	const size_t size = qm_fs_file_get_size( file );
	const char  *path = qm_fs_file_get_path( file );

	AcmBranch *root = acm_load_from_memory( ptr, size, "model", path );
	if ( root == nullptr )
	{
		IO_MODEL_RESULT( result, "failed to load file", IO_MODEL_RESULT_CODE_IO_ERROR );
		return nullptr;
	}

	model = imf_parse( model, root, result );

	acm_branch_destroy( root );

	return model;
}

static void serialize_mesh( AcmBranch *root, const IOModelMesh *mesh, const IOModelVertex *vertices )
{
	const char *c = strrchr( mesh->material, '/' );

	AcmBranch *meshBranch = acm_push_object( root, nullptr );

	//TODO: should go ahead and ensure material exists, and associated texture for material is cooked, etc.
	acm_push_string( meshBranch, "material", mesh->material, false );

	AcmBranch *trianglesBranch = acm_push_array_object( meshBranch, "triangles" );
	for ( unsigned int i = 0; i < mesh->numTriangles; ++i )
	{
		AcmBranch *triangleBranch = acm_push_object( trianglesBranch, nullptr );
		acm_push_array_ui32( triangleBranch, "vertex", mesh->triangles[ i ].indices, 3 );
	}
}

AcmBranch *io_model_imf_serialize( const IOModel *model )
{
	AcmBranch *root = acm_push_object( nullptr, "model" );

	acm_push_ui16( root, "version", IO_MODEL_IMF_VERSION );

	//TODO: this is pretty basic and hard-coded for now... meh...
	//TODO: allow us to store vertex data as other data types besides floats...
	AcmBranch *branch = acm_push_object( root, "vertexFormatDescriptor" );
	acm_push_ui32( branch, "numFloatElements", 8 );
	acm_push_bool( branch, "hasPosition", true );
	acm_push_bool( branch, "hasNormal", true );
	acm_push_bool( branch, "hasUV", true );

	branch = acm_push_array_f32( root, "vertices", nullptr, 0 );
	for ( unsigned int i = 0; i < model->numVertices; ++i )
	{
		const IOModelVertex *vertexIndex = &model->vertices[ i ];
		acm_push_f32( branch, nullptr, vertexIndex->position.x );
		acm_push_f32( branch, nullptr, vertexIndex->position.y );
		acm_push_f32( branch, nullptr, vertexIndex->position.z );
		acm_push_f32( branch, nullptr, vertexIndex->normal.x );
		acm_push_f32( branch, nullptr, vertexIndex->normal.y );
		acm_push_f32( branch, nullptr, vertexIndex->normal.z );
		acm_push_f32( branch, nullptr, vertexIndex->uv.x );
		acm_push_f32( branch, nullptr, vertexIndex->uv.y );
	}

	acm_push_ui8( root, "type", model->type );

#if 0
	if ( model->numBones > 0 )
	{
		branch = acm_push_array_object( root, "bones" );
		for ( unsigned int i = 0; i < model->numBones; ++i )
		{
			serialize_bone( branch, &model->bones[ i ], model );
		}
	}
#endif

	branch = acm_push_array_object( root, "meshes" );
	for ( unsigned int i = 0; i < model->numMeshes; ++i )
	{
		serialize_mesh( branch, &model->meshes[ i ], model->vertices );
	}

	return root;
}
