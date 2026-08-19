// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: SMD loader (pulled from our cook tool, and modified).
// Author:  Mark E. Sowden

#include "plcore/pl_filesystem.h"

#include "qmos/public/qm_os_memory.h"
#include "qmos/public/qm_os_string.h"
#include "qmparse/public/qm_parse.h"

#include "io_model/public/io_model.h"

static constexpr uint8_t      SMD_VERSION       = 1;
static constexpr unsigned int SMD_MAX_MESHES    = IO_MODEL_MAX_MATERIALS;
static constexpr unsigned int SMD_MAX_TRIANGLES = IO_MODEL_MAX_TRIANGLES;
static constexpr unsigned int SMD_MAX_WEIGHTS   = 4;
static constexpr unsigned int SMD_MAX_BONES     = IO_MODEL_MAX_BONES;
static constexpr unsigned int SMD_MAX_FRAMES    = 2048;

static constexpr uint8_t SMD_MAX_TOKEN = 64;

typedef struct SmdFrame
{
	int            bone;
	QmMathVector3f position;
	QmMathVector3f rotation;
} SmdFrame;

typedef struct SmdBone
{
	int             id;
	char            name[ IO_MODEL_MAX_BONE_NAME ];
	struct SmdBone *parent;
	SmdFrame        frames[ SMD_MAX_FRAMES ];
} SmdBone;

typedef struct SmdWeight
{
	SmdBone *node;
	float    value;
} SmdWeight;

typedef struct SmdVertex
{
	SmdBone *defaultBone;

	QmMathVector3f position;
	QmMathVector3f normal;
	QmMathVector2f uv;

	unsigned int numWeights;
	SmdWeight    weights[ SMD_MAX_WEIGHTS ];
} SmdVertex;

typedef struct SmdTriangle
{
	SmdVertex vertices[ 3 ];
} SmdTriangle;

typedef struct SmdMesh
{
	char material[ SMD_MAX_TOKEN ];

	SmdTriangle  triangles[ SMD_MAX_TRIANGLES ];
	unsigned int numTriangles;
} SmdMesh;

typedef struct SmdModel
{
	SmdMesh      meshes[ SMD_MAX_MESHES ];
	unsigned int numMeshes;

	SmdBone      bones[ SMD_MAX_BONES ];
	unsigned int numBones;
} SmdModel;

static SmdModel *parse_smd( SmdModel *self, const char *p, IOModelResult *result )
{
	bool isValidated = false;
	while ( *p != '\0' )
	{
		char token[ SMD_MAX_TOKEN ];
		qm_parse_token( &p, token, sizeof( token ) );
		if ( *token == '\0' || ( token[ 0 ] == '/' && token[ 1 ] == '/' ) )
		{
			qm_parse_skip_line( &p );
			continue;
		}

		if ( !isValidated )
		{
			if ( strcmp( token, "version" ) != 0 )
			{
				IO_MODEL_RESULT( result, "failed to parse version", IO_MODEL_RESULT_CODE_VERSION_ERROR );
				return nullptr;
			}

			int version = qm_parse_integer( &p, nullptr );
			if ( version != SMD_VERSION )
			{
				IO_MODEL_RESULT( result, "invalid version", IO_MODEL_RESULT_CODE_VERSION_ERROR );
				return nullptr;
			}

			qm_parse_skip_line( &p );

			isValidated = true;
			continue;
		}

		// skip nodes for now
		if ( strcmp( token, "nodes" ) == 0 )
		{
			qm_parse_skip_line( &p );
			while ( *p != '\0' )
			{
				qm_parse_token( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
				{
					break;
				}

				// Unique ID number (does not have to be sequential)
				// https://developer.valvesoftware.com/wiki/SMD#Nodes
				const int index = strtol( token, nullptr, 10 );// bone index
				assert( index < SMD_MAX_BONES );
				SmdBone *bone = &self->bones[ index ];
				self->numBones++;

				qm_parse_enclosed( &p, bone->name, sizeof( bone->name ) );// bone name

				const int parent = qm_parse_integer( &p, nullptr );// parent index
				if ( parent >= 0 )
				{
					bone->parent = &self->bones[ parent ];
				}

				qm_parse_skip_line( &p );
			}

			qm_parse_skip_line( &p );
			continue;
		}

		// skip skeleton too...
		if ( strcmp( token, "skeleton" ) == 0 )
		{
			qm_parse_skip_line( &p );

			int frame = 0;
			while ( *p != '\0' )
			{
				qm_parse_token( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
				{
					break;
				}

				if ( strcmp( token, "time" ) == 0 )
				{
					frame = qm_parse_integer( &p, nullptr );
					qm_parse_skip_line( &p );
					continue;
				}

				const int index = ( int ) strtol( token, nullptr, 10 );// bone index
				assert( index < SMD_MAX_BONES && index >= 0 && index < self->numBones );
				SmdBone *bone = &self->bones[ index ];

				//printf( "index: %d, frame: %d, ", index, frame );

				bone->frames[ frame ].position.x = qm_parse_float( &p, nullptr );
				bone->frames[ frame ].position.y = qm_parse_float( &p, nullptr );
				bone->frames[ frame ].position.z = qm_parse_float( &p, nullptr );
				//printf( "pos: %f %f %f, ", bone->frames[ frame ].position.x, bone->frames[ frame ].position.y, bone->frames[ frame ].position.z );

				bone->frames[ frame ].rotation.x = qm_parse_float( &p, nullptr );
				bone->frames[ frame ].rotation.y = qm_parse_float( &p, nullptr );
				bone->frames[ frame ].rotation.z = qm_parse_float( &p, nullptr );
				//printf( "rot: %f %f %f\n", bone->frames[ frame ].rotation.x, bone->frames[ frame ].rotation.y, bone->frames[ frame ].rotation.z );

				qm_parse_skip_line( &p );
			}

			qm_parse_skip_line( &p );
			continue;
		}

		if ( strcmp( token, "triangles" ) == 0 )
		{
			qm_parse_skip_line( &p );
			while ( *p != '\0' )
			{
				/* first need to fetch the material name.
				 * smd spec suggests the extension is ignored, so we'll do the same.
				 */

				qm_parse_token( &p, token, sizeof( token ) );
				if ( strcmp( token, "end" ) == 0 )
				{
					break;
				}

				char material[ SMD_MAX_TOKEN ];
				qm_os_string_copy( material, token, sizeof( material ) );

				qm_parse_skip_line( &p );

				// figure out what slot it falls into

				SmdMesh *smdMesh = nullptr;
				for ( unsigned int i = 0; i < SMD_MAX_MESHES; ++i )
				{
					if ( *self->meshes[ i ].material == '\0' )
					{
						// setup a new slot
						smdMesh = &self->meshes[ i ];
						qm_os_string_copy( smdMesh->material, material, sizeof( smdMesh->material ) );
						qm_os_string_to_lower( smdMesh->material, sizeof( smdMesh->material ) );
						self->numMeshes++;
						break;
					}

					if ( pl_strcasecmp( self->meshes[ i ].material, material ) != 0 )
					{
						continue;
					}

					smdMesh = &self->meshes[ i ];
					break;
				}

				if ( smdMesh == nullptr )
				{
					IO_MODEL_RESULT( result, "failed to fetch mesh for material", IO_MODEL_RESULT_CODE_IO_ERROR );
					return nullptr;
				}

				for ( unsigned int i = 0; i < 3; ++i )
				{
					int boneIndex = qm_parse_integer( &p, nullptr );// bone index
					//smdMesh->triangles[ smdMesh->numTriangles ].vertices[ i ].numWeights = PlParseInteger( &p, nullptr );// num weights

					QmMathVector3f position;
					position.x = qm_parse_float( &p, nullptr );
					position.y = qm_parse_float( &p, nullptr );
					position.z = qm_parse_float( &p, nullptr );

					smdMesh->triangles[ smdMesh->numTriangles ].vertices[ i ].position = position;

					QmMathVector3f normal;
					normal.x = qm_parse_float( &p, nullptr );
					normal.y = qm_parse_float( &p, nullptr );
					normal.z = qm_parse_float( &p, nullptr );

					smdMesh->triangles[ smdMesh->numTriangles ].vertices[ i ].normal = normal;

					QmMathVector2f uv;
					uv.x = qm_parse_float( &p, nullptr );
					uv.y = qm_parse_float( &p, nullptr ) * -1;// inverse, because aaargh

					smdMesh->triangles[ smdMesh->numTriangles ].vertices[ i ].uv = uv;

					qm_parse_skip_line( &p );
				}

				smdMesh->numTriangles++;
			}

			continue;
		}

		printf( "Unhandled token, \"%s\"! Skipping line\n", token );
		qm_parse_skip_line( &p );
	}

	return self;
}

static void smd_destroy( SmdModel *self )
{
	qm_os_memory_free( self );
}

static IOModel *smd_convert( SmdModel *self, IOModel *model, IOModelResult *result )
{
	model->type = self->numBones > 1 ? IO_MODEL_TYPE_SKELETAL : IO_MODEL_TYPE_STATIC;

	// setup the materials list
	model->numMaterials = self->numMeshes;
	assert( model->numMaterials < IO_MODEL_MAX_MATERIALS );
	for ( unsigned int i = 0; i < model->numMaterials; ++i )
	{
		const SmdMesh *srcMesh    = &self->meshes[ i ];
		const size_t   length     = strlen( srcMesh->material );
		model->materialPaths[ i ] = QM_OS_MEMORY_NEW_( char, length + 1 );
		strcpy( model->materialPaths[ i ], srcMesh->material );
	}

	for ( unsigned int i = 0; i < model->numMeshes; ++i )
	{
		const SmdMesh *srcMesh = &self->meshes[ i ];
		IOModelMesh   *dstMesh = &model->meshes[ i ];
		dstMesh->materialIndex = srcMesh->
	}
}

IOModel *io_model_smd_load_( IOModel *model, QmFsFile *file, IOModelResult *result )
{
	if ( PlCacheFile( file ) == nullptr )
	{
		IO_MODEL_RESULT( result, "failed to cache file", IO_MODEL_RESULT_CODE_IO_ERROR );
		return nullptr;
	}

	const char *p = ( char * ) qm_fs_file_get_data( file );
	if ( *p == '\0' )
	{
		IO_MODEL_RESULT( result, "smd is empty", IO_MODEL_RESULT_CODE_IO_ERROR );
		return nullptr;
	}

	SmdModel *smdModel = QM_OS_MEMORY_NEW( SmdModel );
	if ( parse_smd( smdModel, p, result ) == nullptr )
	{
		model = nullptr;
	}

	smd_convert( smdModel, model, result );
	smd_destroy( smdModel );

	return model;
}
