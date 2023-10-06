// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: RFC loader
// Author:  Mark E. Sowden

#include "model_rfm.h"

#include "yin/core_fs.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static const unsigned int RFM_MAGIC = 0x87128712;

static const unsigned int RFM_VERSION_RF = 1;
static const unsigned int RFM_VERSION_RF2 = 10;

// These only apply to version 1
#define RFC_CHUNK_BONE       PL_MAGIC_TO_NUM_R( 'B', 'O', 'N', 'E' )
#define RFC_CHUNK_ATTACHMENT PL_MAGIC_TO_NUM_R( 'D', 'U', 'M', 'B' )
#define RFC_CHUNK_COLLISION  PL_MAGIC_TO_NUM_R( 'C', 'S', 'P', 'H' )
#define RFC_CHUNK_MATERIAL   PL_MAGIC_TO_NUM( 'D', '3', 0x13, 0x11 )
#define RFC_CHUNK_MESH       PL_MAGIC_TO_NUM( 0x10, 0x11, 0x25, 0x87 )

static AclModelRfm *parse_rfm_chunk_material( AclModelRfm *model, PLFile *file )
{
	unsigned int numMaterials = PL_READUINT32( file, false, NULL );
	assert( numMaterials == model->numMaterials );
	if ( numMaterials != model->numMaterials )
	{
		PRINT_WARNING( "Unexpected number of materials in material chunk!\n" );
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

static AclModelRfm *deserialize_rfm_v1( PLFile *file )
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
			case RFC_CHUNK_MATERIAL:
				parse_rfm_chunk_material( model, file );
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

static AclModelRfm *deserialize_rfm_v10( PLFile *file )
{
	// "We have a perfectly good format, but how can we make it better?"
	// "I know, let's just make the same format, but different."
	// - Volition developer, ca. 2002/2003

	bool hasBones = ( bool ) PL_READUINT32( file, false, NULL );
	PRINT_DEBUG( "has bones: %s\n", hasBones ? "true" : "false" );
	// if true, usually an RFC, otherwise a RFM

	return NULL;
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
	if ( version == RFM_VERSION_RF )
		return deserialize_rfm_v1( file );
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
