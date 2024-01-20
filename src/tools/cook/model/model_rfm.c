// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: RFM loader
//			Status - unfinished
// Author:  Mark E. Sowden

#include "../cook.h"

#include "model.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

#define RFM_MAX_MESHES            64
#define RFM_MAX_MATERIALS         32
#define RFM_MAX_BONES             64// hard limit of 49 in RF
#define RFM_MAX_COLLISION_SPHERES 64

#define RFX_FLAG_TRANSPARENT 8

typedef struct RfcMaterial
{
	char diffuseTexture[ 32 ];
	char reflectionTexture[ 32 ];

	float specular;
	float gloss;
	float reflectivity;
	float illumination;

	unsigned int flags;
} RfcMaterial;

typedef struct RfmMesh
{
	PLVector3 boundsMins;
	PLVector3 boundsMaxs;
	PLVector3 boundsOrigin;
	float boundsRadius;

	unsigned int flags;

	unsigned int numChunks;
} RfmMesh;

typedef struct AclModelRfcBone
{
	char name[ 24 ];
	PLQuaternion rotation;
	PLVector3 transform;
	struct AclModelRfcBone *parent;
} AclModelRfcBone;

typedef struct AclModelRfmCollisionSphere
{
	char name[ 24 ];
	float radius;
	PLVector3 transform;
	struct AclModelRfmCollisionSphere *parent;
} AclModelRfmCollisionSphere;

typedef struct RfmModel
{
	unsigned int numLods;

	unsigned int numMeshes;
	RfmMesh meshes[ RFM_MAX_MESHES ];

	unsigned int numCollisionSpheres;
	AclModelRfmCollisionSphere collisionSpheres[ RFM_MAX_COLLISION_SPHERES ];

	unsigned int numAttachments;

	unsigned int numMaterials;
	RfcMaterial materials[ RFM_MAX_MATERIALS ];

	unsigned int numBones;
	AclModelRfcBone bones[ RFM_MAX_BONES ];
	AclModelRfcBone *rootBone;
} RfmModel;

static const unsigned int RFM_MAGIC = 0x87128712;
static const unsigned int RFM_MAGIC_PUN = 0x87128713;

static const unsigned int RFM_VERSION_RF1 = 1;
static const unsigned int RFM_VERSION_RF2 = 10;
static const unsigned int RFM_VERSION_PUN = 21;// first magic byte is changed too

// These only apply to version 1
#define RFM_CHUNK_BONE       PL_MAGIC_TO_NUM( 'E', 'N', 'O', 'B' )
#define RFM_CHUNK_ATTACHMENT PL_MAGIC_TO_NUM( 'D', 'M', 'U', 'D' )
#define RFM_CHUNK_COLLISION  PL_MAGIC_TO_NUM( 'H', 'P', 'S', 'C' )
#define RFM_CHUNK_MATERIAL   PL_MAGIC_TO_NUM( 'D', '3', 0x13, 0x11 )
#define RFM_CHUNK_MESH       PL_MAGIC_TO_NUM( 0x10, 0x11, 0x25, 0x87 )

void acl_model_rfm_destroy_( RfmModel *model );

#if 0 // return to this later...

static RfmModel *parse_rfm_chunk_collision( RfmModel *model, PLFile *file, unsigned int version )
{
	for ( unsigned int i = 0; i < model->numCollisionSpheres; ++i )
	{
		if ( PlReadFile( file, model->collisionSpheres[ i ].name, sizeof( char ), 24 ) != 24 )
		{
			WARN( "Failed to read collision sphere name (%s)!\n", PlGetError() );
			return NULL;
		}

		unsigned int parent = PL_READUINT32( file, false, NULL );
		//assert( parent < model->numCollisionSpheres );
		if ( parent >= model->numCollisionSpheres )
		{
			WARN( "Invalid parent index for collision sphere (%u >= %u)!\n", parent, model->numCollisionSpheres );
			parent = 0;
		}

		printf( "sphere name: %s\n", model->collisionSpheres[ i ].name );
		model->collisionSpheres[ i ].parent = &model->collisionSpheres[ parent ];
		model->collisionSpheres[ i ].transform = ss_acl_fs_parse_vector( file );
		model->collisionSpheres[ i ].radius = ss_acl_fs_parse_float( file );
	}

	return model;
}

static RfmModel *parse_rfm_chunk_material( RfmModel *model, PLFile *file, unsigned int version )
{
	unsigned int numMaterials = PL_READUINT32( file, false, NULL );
	assert( numMaterials == model->numMaterials );
	if ( numMaterials != model->numMaterials )
	{
		WARN( "Unexpected number of materials in material chunk (%u != %u)!\n", numMaterials, model->numMaterials );
		return NULL;
	}

	for ( unsigned int i = 0; i < model->numMaterials; ++i )
	{
		PlReadFile( file, model->materials[ i ].diffuseTexture, sizeof( char ), 32 );
		if ( strlen( model->materials[ i ].diffuseTexture ) == 0 )
			break;

		printf( "diffuse: %s\n", model->materials[ i ].diffuseTexture );
		model->materials[ i ].illumination = ss_acl_fs_parse_float( file );
		printf( "illumination: %f\n", model->materials[ i ].illumination );
		model->materials[ i ].specular = ss_acl_fs_parse_float( file );
		printf( "specular: %f\n", model->materials[ i ].specular );
		model->materials[ i ].gloss = ss_acl_fs_parse_float( file );
		printf( "gloss: %f\n", model->materials[ i ].gloss );
		model->materials[ i ].reflectivity = ss_acl_fs_parse_float( file );
		printf( "reflectivity: %f\n", model->materials[ i ].reflectivity );

		PlReadFile( file, model->materials[ i ].reflectionTexture, sizeof( char ), 32 );
		printf( "reflection: %s\n", *model->materials[ i ].reflectionTexture != '\0' ? model->materials[ i ].reflectionTexture : "none" );

		model->materials[ i ].flags = PL_READUINT32( file, false, NULL );
	}

	return model;
}

static RfmModel *parse_rfm_chunk_bone( RfmModel *model, PLFile *file, unsigned int version )
{
	model->numBones = PL_READUINT32( file, false, NULL );
	assert( model->numBones < RFM_MAX_BONES );
	if ( model->numBones >= RFM_MAX_BONES )
	{
		WARN( "Reached internal bone limit: %u >= %u\n", model->numBones, RFM_MAX_BONES );
		model->numBones = RFM_MAX_BONES - 1;
	}

	printf( "num bones: %u\n", model->numBones );
	for ( unsigned int i = 0; i < model->numBones; ++i )
	{
		PlReadFile( file, model->bones[ i ].name, sizeof( char ), 24 );
		printf( "name: %s\n", model->bones[ i ].name );

		model->bones[ i ].rotation = ss_acl_fs_parse_vector4( file );
		printf( "rotation: %s\n", PlPrintQuaternion( &model->bones[ i ].rotation ) );
		model->bones[ i ].transform = ss_acl_fs_parse_vector( file );
		printf( "transform: %s\n", PlPrintVector3( &model->bones[ i ].transform, PL_VAR_F32 ) );
		unsigned int parent = PL_READUINT32( file, false, NULL );
		printf( "parent: %u\n", parent );
		if ( parent >= RFM_MAX_BONES )
		{
			assert( model->rootBone == NULL );
			if ( model->rootBone != NULL )
				WARN( "Encountered second bone with invalid parent! Root may be incorrect.\n" );

			model->rootBone = &model->bones[ i ];
		}
		else
			model->bones[ i ].parent = &model->bones[ parent ];
	}

	return model;
}

static RfmModel *parse_rfm_chunk_mesh( RfmModel *model, PLFile *file, unsigned int version )
{
	RfmMesh *mesh = &model->meshes[ model->numMeshes ];

	float lodDistance = ss_acl_fs_parse_float( file );
	printf( "lod distance: %f\n", lodDistance );

	// Not 100% sure on this one, yet
	unsigned int numMaterials = PL_READUINT32( file, false, NULL );
	printf( "num materials: %u\n", numMaterials );
	if ( numMaterials < 3 )
		return model;

	mesh->flags = PL_READUINT32( file, false, NULL );
	printf( "flags: %u\n", model->meshes[ model->numMeshes ].flags );
	unsigned int numOriginalVecs = PL_READUINT32( file, false, NULL );
	printf( "num original vecs: %u\n", numOriginalVecs );

	mesh->boundsMaxs = ss_acl_fs_parse_vector( file );
	mesh->boundsMins = ss_acl_fs_parse_vector( file );
	mesh->boundsOrigin = ss_acl_fs_parse_vector( file );
	mesh->boundsRadius = ss_acl_fs_parse_float( file );

	unsigned int dataBlockSize = PL_READUINT32( file, false, NULL );
	PlFileSeek( file, dataBlockSize, PL_SEEK_CUR );

	mesh->numChunks = PL_READUINT16( file, false, NULL );
	printf( "num chunks: %u\n", mesh->numChunks );

	model->numMeshes++;
	return model;
}

static RfmModel *deserialize_rfm_v1( PLFile *file, unsigned int version )
{
	// This always appears to be equal to 1 :shrug:
	if ( PL_READUINT32( file, false, NULL ) != 1 )
	{
		WARN( "Not a valid RFC file!\n" );
		return NULL;
	}

	RfmModel *model = PL_NEW( RfmModel );
	unsigned int numLodMeshes = PL_READUINT32( file, false, NULL );
	printf( "num lod meshes: %u\n", numLodMeshes );
	model->numLods = PL_READUINT32( file, false, NULL );
	printf( "num lods: %u\n", model->numLods );
	model->numCollisionSpheres = PL_READUINT32( file, false, NULL );
	printf( "num spheres: %u\n", model->numCollisionSpheres );
	model->numAttachments = PL_READUINT32( file, false, NULL );
	printf( "num attachments: %u\n", model->numAttachments );
	model->numMaterials = PL_READUINT32( file, false, NULL );
	printf( "num materials: %u\n", model->numMaterials );

	// Yet another chunk-based format, wheee...
	for ( ;; )
	{
		unsigned int chunkTag = PL_READUINT32( file, false, NULL );
		unsigned int chunkSize = PL_READUINT32( file, false, NULL );
		if ( chunkTag == 0 )
			break;

		PLFileOffset offset = PlGetFileOffset( file );
		PLFileOffset nextChunk = offset + chunkSize;

		switch ( chunkTag )
		{
			case RFM_CHUNK_MATERIAL:
				parse_rfm_chunk_material( model, file, version );
				break;
			case RFM_CHUNK_BONE:
				parse_rfm_chunk_bone( model, file, version );
				break;
			case RFM_CHUNK_MESH:
				parse_rfm_chunk_mesh( model, file, version );
				break;
			case RFM_CHUNK_COLLISION:
				parse_rfm_chunk_collision( model, file, version );
				break;
			default:
			{
				static char tagName[ 5 ];
				if ( isalpha( *( char * ) &chunkTag ) )
					strncpy( tagName, ( char * ) &chunkTag, 4 );
				else
					strncpy( tagName, "none", 4 );

				WARN( "Skipping unknown chunk (%x/%s : %u)\n", chunkTag, tagName, offset );
				break;
			}
		}

		// always do this afterwards, just on the off-chance the chunk failed to read
		PlFileSeek( file, nextChunk, PL_SEEK_SET );
	}

	return model;
}

static unsigned int seek_next( PLFile *file )
{
	static const unsigned int BLOCK_SIZE = 64;
	PlFileSeek( file, ( PLFileOffset ) ceil( ( double ) PlGetFileOffset( file ) / BLOCK_SIZE ) * BLOCK_SIZE, PL_SEEK_SET );
	printf( "Seeked to offset %lu\n", PlGetFileOffset( file ) );
	return PlGetFileOffset( file );
}

static RfmModel *deserialize_rfm_v10( PLFile *file )
{
	// "We have a perfectly good format, but how can we make it better?"
	// "I know, let's just make the same format, but different."
	// - Volition developer, ca. 2002/2003

	bool hasBones = ( bool ) PL_READUINT32( file, false, NULL );// could actually be a flag...
	printf( "has bones: %s\n", hasBones ? "true" : "false" );
	// if true, usually an RFC, otherwise a RFM

	RfmModel *model = PL_NEW( RfmModel );
	printf( "a: %u\n", PL_READUINT32( file, false, NULL ) );// always 16??

	model->numCollisionSpheres = PL_READUINT32( file, false, NULL );
	printf( "num spheres: %u\n", model->numCollisionSpheres );
	model->numAttachments = PL_READUINT32( file, false, NULL );
	printf( "num attachments: %u\n", model->numAttachments );
	model->numBones = PL_READUINT32( file, false, NULL );
	printf( "num bones: %u\n", model->numBones );
	model->numMaterials = PL_READUINT32( file, false, NULL );
	printf( "num materials: %u\n", model->numMaterials );
	printf( "e: %u\n", PL_READUINT32( file, false, NULL ) );// always 1??
	printf( "f: %u\n", PL_READUINT32( file, false, NULL ) );// always 1??

	seek_next( file );
	if ( parse_rfm_chunk_collision( model, file, 10 ) == NULL )
	{
		WARN( "Failed to read in collision spheres!\n" );
		acl_model_rfm_destroy_( model );
		return NULL;
	}

	seek_next( file );

	return model;
}

RfmModel *acl_model_rfm_parse_file_( PLFile *file )
{
	unsigned int magic = PL_READUINT32( file, false, NULL );
	if ( magic != RFM_MAGIC )
	{
		WARN( "Not an RFM file!\n" );
		return NULL;
	}

	unsigned int version = PL_READUINT32( file, false, NULL );
	printf( "RFM version %u\n", version );
	if ( version == RFM_VERSION_RF1 )
		return deserialize_rfm_v1( file, version );
	else if ( version == RFM_VERSION_RF2 )
		return deserialize_rfm_v10( file );

	WARN( "Unsupported RFM version (%u)!\n", version );
	return NULL;
}

RfmModel *acl_model_rfm_load_file_( const char *filename )
{
	PLFile *file = PlOpenFile( filename, false );
	if ( file == NULL )
	{
		WARN( "Failed to load RFM model (%s): %s\n", filename, PlGetError() );
		return NULL;
	}

	RfmModel *model = acl_model_rfm_parse_file_( file );

	PlCloseFile( file );
	return model;
}

void acl_model_rfm_destroy_( RfmModel *model )
{
	if ( model == NULL )
		return;

	PL_DELETE( model );
}

#if 0
void acl_model_rfm_test_command_( unsigned int argc, char **argv )
{
	static const char *MODELS[] =
	        {
	                // Red Faction
	                "miner.rfc",
	                "riot_guard.rfc",
	                // Red Faction II
	                "fodder_cop.rfc",
	                "security_guard.rfc",
	                "powerup_health.rfm",
	                "wep_shotgun.rfm",
	        };
	static const unsigned int NUM_MODELS = PL_ARRAY_ELEMENTS( MODELS );

	for ( unsigned int i = 0; i < NUM_MODELS; ++i )
	{
		printf( "model: %s\n", MODELS[ i ] );
		RfmModel *model = acl_model_rfm_load_file_( MODELS[ i ] );
		if ( model == NULL )
			continue;

		acl_model_rfm_destroy_( model );
	}
}
#endif

#endif

/////////////////////////////////////////////////////////////////////////////////////
// Public
