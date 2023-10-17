// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: RFM loader
// Author:  Mark E. Sowden

#include "model_rfm.h"

#include "yin/core_fs.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

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

static AclModelRfm *parse_rfm_chunk_collision( AclModelRfm *model, PLFile *file, unsigned int version )
{
	for ( unsigned int i = 0; i < model->numCollisionSpheres; ++i )
	{
		if ( PlReadFile( file, model->collisionSpheres[ i ].name, sizeof( char ), 24 ) != 24 )
		{
			PRINT_WARNING( "Failed to read collision sphere name (%s)!\n", PlGetError() );
			return NULL;
		}

		unsigned int parent = PL_READUINT32( file, false, NULL );
		//assert( parent < model->numCollisionSpheres );
		if ( parent >= model->numCollisionSpheres )
		{
			PRINT_WARNING( "Invalid parent index for collision sphere (%u >= %u)!\n", parent, model->numCollisionSpheres );
			parent = 0;
		}

		PRINT_DEBUG( "sphere name: %s\n", model->collisionSpheres[ i ].name );
		model->collisionSpheres[ i ].parent = &model->collisionSpheres[ parent ];
		model->collisionSpheres[ i ].transform = acl_fs_parse_vector( file );
		model->collisionSpheres[ i ].radius = acl_fs_parse_float( file );
	}

	return model;
}

static AclModelRfm *parse_rfm_chunk_material( AclModelRfm *model, PLFile *file, unsigned int version )
{
	unsigned int numMaterials = PL_READUINT32( file, false, NULL );
	assert( numMaterials == model->numMaterials );
	if ( numMaterials != model->numMaterials )
	{
		PRINT_WARNING( "Unexpected number of materials in material chunk (%u != %u)!\n", numMaterials, model->numMaterials );
		return NULL;
	}

	for ( unsigned int i = 0; i < model->numMaterials; ++i )
	{
		PlReadFile( file, model->materials[ i ].diffuseTexture, sizeof( char ), 32 );
		if ( strlen( model->materials[ i ].diffuseTexture ) == 0 )
			break;

		PRINT_DEBUG( "diffuse: %s\n", model->materials[ i ].diffuseTexture );
		model->materials[ i ].illumination = acl_fs_parse_float( file );
		PRINT_DEBUG( "illumination: %f\n", model->materials[ i ].illumination );
		model->materials[ i ].specular = acl_fs_parse_float( file );
		PRINT_DEBUG( "specular: %f\n", model->materials[ i ].specular );
		model->materials[ i ].gloss = acl_fs_parse_float( file );
		PRINT_DEBUG( "gloss: %f\n", model->materials[ i ].gloss );
		model->materials[ i ].reflectivity = acl_fs_parse_float( file );
		PRINT_DEBUG( "reflectivity: %f\n", model->materials[ i ].reflectivity );

		PlReadFile( file, model->materials[ i ].reflectionTexture, sizeof( char ), 32 );
		PRINT_DEBUG( "reflection: %s\n", *model->materials[ i ].reflectionTexture != '\0' ? model->materials[ i ].reflectionTexture : "none" );

		model->materials[ i ].flags = PL_READUINT32( file, false, NULL );
	}

	return model;
}

static AclModelRfm *parse_rfm_chunk_bone( AclModelRfm *model, PLFile *file, unsigned int version )
{
	model->numBones = PL_READUINT32( file, false, NULL );
	assert( model->numBones < ACL_MODEL_RFM_MAX_BONES );
	if ( model->numBones >= ACL_MODEL_RFM_MAX_BONES )
	{
		PRINT_WARNING( "Reached internal bone limit: %u >= %u\n", model->numBones, ACL_MODEL_RFM_MAX_BONES );
		model->numBones = ACL_MODEL_RFM_MAX_BONES - 1;
	}

	PRINT_DEBUG( "num bones: %u\n", model->numBones );
	for ( unsigned int i = 0; i < model->numBones; ++i )
	{
		PlReadFile( file, model->bones[ i ].name, sizeof( char ), 24 );
		PRINT_DEBUG( "name: %s\n", model->bones[ i ].name );

		model->bones[ i ].rotation = acl_fs_parse_vector4( file );
		PRINT_DEBUG( "rotation: %s\n", PlPrintQuaternion( &model->bones[ i ].rotation ) );
		model->bones[ i ].transform = acl_fs_parse_vector( file );
		PRINT_DEBUG( "transform: %s\n", PlPrintVector3( &model->bones[ i ].transform, PL_VAR_F32 ) );
		unsigned int parent = PL_READUINT32( file, false, NULL );
		PRINT_DEBUG( "parent: %u\n", parent );
		if ( parent >= ACL_MODEL_RFM_MAX_BONES )
		{
			assert( model->rootBone == NULL );
			if ( model->rootBone != NULL )
				PRINT_WARNING( "Encountered second bone with invalid parent! Root may be incorrect.\n" );

			model->rootBone = &model->bones[ i ];
		}
		else
			model->bones[ i ].parent = &model->bones[ parent ];
	}

	return model;
}

static AclModelRfm *parse_rfm_chunk_mesh( AclModelRfm *model, PLFile *file, unsigned int version )
{
	AclModelRfmMesh *mesh = &model->meshes[ model->numMeshes ];

	float lodDistance = acl_fs_parse_float( file );
	PRINT_DEBUG( "lod distance: %f\n", lodDistance );

	// Not 100% sure on this one, yet
	unsigned int numMaterials = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num materials: %u\n", numMaterials );
	if ( numMaterials < 3 )
		return model;

	mesh->flags = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "flags: %u\n", model->meshes[ model->numMeshes ].flags );
	unsigned int numOriginalVecs = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num original vecs: %u\n", numOriginalVecs );

	mesh->boundsMaxs = acl_fs_parse_vector( file );
	mesh->boundsMins = acl_fs_parse_vector( file );
	mesh->boundsOrigin = acl_fs_parse_vector( file );
	mesh->boundsRadius = acl_fs_parse_float( file );

	unsigned int dataBlockSize = PL_READUINT32( file, false, NULL );
	PlFileSeek( file, dataBlockSize, PL_SEEK_CUR );

	mesh->numChunks = PL_READUINT16( file, false, NULL );
	PRINT_DEBUG( "num chunks: %u\n", mesh->numChunks );

	model->numMeshes++;
	return model;
}

static AclModelRfm *deserialize_rfm_v1( PLFile *file, unsigned int version )
{
	// This always appears to be equal to 1 :shrug:
	if ( PL_READUINT32( file, false, NULL ) != 1 )
	{
		PRINT_WARNING( "Not a valid RFC file!\n" );
		return NULL;
	}

	AclModelRfm *model = PL_NEW( AclModelRfm );
	unsigned int numLodMeshes = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num lod meshes: %u\n", numLodMeshes );
	model->numLods = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num lods: %u\n", model->numLods );
	model->numCollisionSpheres = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num spheres: %u\n", model->numCollisionSpheres );
	model->numAttachments = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num attachments: %u\n", model->numAttachments );
	model->numMaterials = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num materials: %u\n", model->numMaterials );

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

				PRINT_WARNING( "Skipping unknown chunk (%x/%s : %u)\n", chunkTag, tagName, offset );
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
	PRINT_DEBUG( "Seeked to offset %lu\n", PlGetFileOffset( file ) );
	return PlGetFileOffset( file );
}

static AclModelRfm *deserialize_rfm_v10( PLFile *file )
{
	// "We have a perfectly good format, but how can we make it better?"
	// "I know, let's just make the same format, but different."
	// - Volition developer, ca. 2002/2003

	bool hasBones = ( bool ) PL_READUINT32( file, false, NULL );// could actually be a flag...
	PRINT_DEBUG( "has bones: %s\n", hasBones ? "true" : "false" );
	// if true, usually an RFC, otherwise a RFM

	AclModelRfm *model = PL_NEW( AclModelRfm );
	PRINT_DEBUG( "a: %u\n", PL_READUINT32( file, false, NULL ) );// always 16??

	model->numCollisionSpheres = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num spheres: %u\n", model->numCollisionSpheres );
	model->numAttachments = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num attachments: %u\n", model->numAttachments );
	model->numBones = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num bones: %u\n", model->numBones );
	model->numMaterials = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "num materials: %u\n", model->numMaterials );
	PRINT_DEBUG( "e: %u\n", PL_READUINT32( file, false, NULL ) );// always 1??
	PRINT_DEBUG( "f: %u\n", PL_READUINT32( file, false, NULL ) );// always 1??

	seek_next( file );
	if ( parse_rfm_chunk_collision( model, file, 10 ) == NULL )
	{
		PRINT_WARNING( "Failed to read in collision spheres!\n" );
		acl_model_rfm_destroy_( model );
		return NULL;
	}

	seek_next( file );

	return model;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

AclModelRfm *acl_model_rfm_parse_file_( PLFile *file )
{
	unsigned int magic = PL_READUINT32( file, false, NULL );
	if ( magic != RFM_MAGIC )
	{
		PRINT_WARNING( "Not an RFM file!\n" );
		return NULL;
	}

	unsigned int version = PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "RFM version %u\n", version );
	if ( version == RFM_VERSION_RF1 )
		return deserialize_rfm_v1( file, version );
	else if ( version == RFM_VERSION_RF2 )
		return deserialize_rfm_v10( file );

	PRINT_WARNING( "Unsupported RFM version (%u)!\n", version );
	return NULL;
}

AclModelRfm *acl_model_rfm_load_file_( const char *filename )
{
	PLFile *file = PlOpenFile( filename, false );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to load RFM model (%s): %s\n", filename, PlGetError() );
		return NULL;
	}

	AclModelRfm *model = acl_model_rfm_parse_file_( file );

	PlCloseFile( file );
	return model;
}

void acl_model_rfm_destroy_( AclModelRfm *model )
{
	if ( model == NULL )
		return;

	PL_DELETE( model );
}

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
		PRINT_DEBUG( "model: %s\n", MODELS[ i ] );
		AclModelRfm *model = acl_model_rfm_load_file_( MODELS[ i ] );
		if ( model == NULL )
			continue;

		acl_model_rfm_destroy_( model );
	}
}
