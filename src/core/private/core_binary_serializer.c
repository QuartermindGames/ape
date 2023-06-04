// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>

#include "core_private.h"
#include "core_binary_serializer.h"

// Carried over from Compton, 2022-03-16

typedef struct Serializer
{
	FILE    *file;
	uint32_t version;
} Serializer;

#define SERIALIZER_FORMAT_MAGIC   PL_MAGIC_TO_NUM( 'G', 'D', 'S', '1' )
#define SERIALIZER_FORMAT_VERSION 20200629

Serializer *Serializer_Create( const char *path, SerializerMode mode )
{
	FILE *file = fopen( path, mode == SERIALIZER_MODE_READ ? "rb" : "wb" );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to open \"%s\" for read/write operations, state will not be correctly restored!\n", path );
		return NULL;
	}

	uint32_t version;

	// If we're reading, make sure the dataset is good
	if ( mode == SERIALIZER_MODE_READ )
	{
		uint32_t magic;
		fread( &magic, sizeof( uint32_t ), 1, file );
		if ( magic != SERIALIZER_FORMAT_MAGIC )
		{
			PRINT_WARNING( "Invalid storage header, \"%s\", expected \"GDS1\"!\n", magic );

			// Clear out the file so we don't do anything with it
			fclose( file );
			return NULL;
		}

		// Read in the version, we can use this to handle any incompatibilities etc.
		fread( &version, sizeof( uint32_t ), 1, file );
	}
	else
	{
		// Otherwise, just write out the header
		fwrite( ( const void * ) SERIALIZER_FORMAT_MAGIC, sizeof( uint32_t ), 1, file );
		fwrite( ( const void * ) SERIALIZER_FORMAT_VERSION, sizeof( uint32_t ), 1, file );
	}

	Serializer *serializer = PL_NEW( Serializer );
	serializer->file = file;
	serializer->version = version;

	return serializer;
}

void Serializer_Destroy( Serializer *serializer )
{
	if ( serializer == NULL )
		return;

	if ( serializer->file != NULL )
		fclose( serializer->file );

	PlFree( serializer );
}

bool Serializer_ValidateDataFormat( Serializer *serializer, uint8_t target )
{
	uint8_t format = fgetc( serializer->file );
	if ( format != target )
	{
		PRINT_WARNING( "Invalid data format found, expected \"%d\" but got \"%d\"!\n", target, format );
		return false;
	}

	return true;
}

void Serializer_WriteI32( Serializer *serializer, int32_t var )
{
	fputc( SERIALIZER_DATA_FORMAT_I32, serializer->file );
	fwrite( &var, sizeof( int32_t ), 1, serializer->file );
}

void Serializer_WriteF32( Serializer *serializer, float var )
{
	fputc( SERIALIZER_DATA_FORMAT_F32, serializer->file );
	fwrite( &var, sizeof( float ), 1, serializer->file );
}

void Serializer_WriteString( Serializer *serializer, const char *var )
{
	fputc( SERIALIZER_DATA_FORMAT_STRING, serializer->file );

	// Write out the length
	uint32_t length = strlen( var );
	fwrite( &length, sizeof( uint32_t ), 1, serializer->file );

	// And now write out the string itself
	fwrite( var, sizeof( char ), length, serializer->file );
}

void Serializer_WriteVector2( Serializer *serializer, const PLVector2 *var )
{
	fputc( SERIALIZER_DATA_FORMAT_VECTOR2, serializer->file );
	fwrite( var, sizeof( PLVector2 ), 1, serializer->file );
}

void Serializer_WriteVector3( Serializer *serializer, const PLVector3 *var )
{
	fputc( SERIALIZER_DATA_FORMAT_VECTOR3, serializer->file );
	fwrite( var, sizeof( PLVector3 ), 1, serializer->file );
}

int32_t Serializer_ReadI32( Serializer *serializer )
{
	if ( !Serializer_ValidateDataFormat( serializer, SERIALIZER_DATA_FORMAT_I32 ) )
		return 0;

	int32_t var;
	fread( &var, sizeof( int32_t ), 1, serializer->file );
	return var;
}

float Serializer_ReadF32( Serializer *serializer )
{
	if ( !Serializer_ValidateDataFormat( serializer, SERIALIZER_DATA_FORMAT_F32 ) )
		return 0.0f;

	float var;
	fread( &var, sizeof( float ), 1, serializer->file );
	return var;
}

const char *Serializer_ReadString( Serializer *serializer, char *dst, size_t dstLength )
{
	if ( !Serializer_ValidateDataFormat( serializer, SERIALIZER_DATA_FORMAT_STRING ) )
		return NULL;

	uint32_t length;
	fread( &length, sizeof( uint32_t ), 1, serializer->file );

	char *c = PL_NEW_( char, length );
	fread( c, sizeof( char ), length, serializer->file );

	snprintf( dst, dstLength, "%s", c );

	PlFree( c );

	return dst;
}

PLVector2 Serializer_ReadVector2( Serializer *serializer )
{
	if ( !Serializer_ValidateDataFormat( serializer, SERIALIZER_DATA_FORMAT_VECTOR2 ) )
		return pl_vecOrigin2;

	PLVector2 vector;
	fread( &vector, sizeof( PLVector2 ), 1, serializer->file );
	return vector;
}

PLVector3 Serializer_ReadVector3( Serializer *serializer )
{
	if ( !Serializer_ValidateDataFormat( serializer, SERIALIZER_DATA_FORMAT_VECTOR3 ) )
		return pl_vecOrigin3;

	PLVector3 vector;
	fread( &vector, sizeof( PLVector3 ), 1, serializer->file );
	return vector;
}
