// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "audio.h"

#define CHUNK_RIFF PL_MAGIC_TO_NUM( 'R', 'I', 'F', 'F' )
#define CHUNK_DATA PL_MAGIC_TO_NUM( 'd', 'a', 't', 'd' )
#define CHUNK_FMT  PL_MAGIC_TO_NUM( ' ', 'f', 'm', 't' )
#define CHUNK_WAVE PL_MAGIC_TO_NUM( 'W', 'A', 'V', 'E' )
#define CHUNK_XWMA PL_MAGIC_TO_NUM( 'X', 'W', 'M', 'A' )
#define CHUNK_DPDS PL_MAGIC_TO_NUM( 'd', 'p', 'd', 's' )

static bool FindChunk( PLFile *file, uint32_t fourCC, uint32_t *chunkSize, uint32_t *chunkDataPosition )
{
	/* throw us back to the start of the file */
	PlRewindFile( file );

	uint32_t offset = 0;

	/* now iterate through to find the chunk we're after */
	bool status = true;
	while ( status )
	{
		uint32_t type = PlReadInt32( file, false, &status );
		uint32_t size = PlReadInt32( file, false, &status );

		if ( type == CHUNK_RIFF )
			size = 4;
		else if ( !PlFileSeek( file, ( long ) size, PL_SEEK_CUR ) )
			break;

		offset += sizeof( uint32_t ) * 2;

		if ( type == fourCC )
		{
			*chunkSize         = size;
			*chunkDataPosition = offset;
			return true;
		}

		offset += size;
	}

	PRINT_WARNING( "Failed to find chunk: %X\n", fourCC );
	return false;
}

static bool ReadChunkData( PLFile *file, void *buffer, uint32_t bufferSize, uint32_t bufferOffset )
{
	if ( !PlFileSeek( file, ( long ) bufferOffset, PL_SEEK_SET ) )
	{
		PRINT_WARNING( "Failed to seek to chunk location: %s\n", PlGetError() );
		return false;
	}

	if ( PlReadFile( file, buffer, bufferSize, 1 ) != 1 )
	{
		PRINT_WARNING( "Failed to read in chunk data: %s\n", PlGetError() );
		return false;
	}

	return true;
}

static void *ParseWav( PLFile *file, YNCoreAudioWaveFormat *waveFormatEx, unsigned int *bufferSize )
{
	/* ensure the type is valid */
	uint32_t chunkSize;
	uint32_t chunkPosition;
	if ( FindChunk( file, CHUNK_RIFF, &chunkSize, &chunkPosition ) )
	{
		uint32_t fileType;
		if ( !ReadChunkData( file, &fileType, sizeof( uint32_t ), chunkPosition ) )
			return NULL;

		if ( fileType != CHUNK_WAVE )
		{
			PRINT_WARNING( "Unexpected file type!\n" );
			return NULL;
		}
	}
	else
	{
		return NULL;
	}

	/* fetch the format */
	if ( FindChunk( file, CHUNK_FMT, &chunkSize, &chunkPosition ) )
	{
		if ( chunkSize <= sizeof( YNCoreAudioWaveFormat ) )
		{
			PRINT_WARNING( "Chunk size is too small to hold format data!\n" );
			return NULL;
		}

		if ( !ReadChunkData( file, waveFormatEx, sizeof( YNCoreAudioWaveFormat ), chunkPosition ) )
			return NULL;
	}
	else
		return NULL;

	/* and finally the data */
	if ( FindChunk( file, CHUNK_DATA, &chunkSize, &chunkPosition ) )
	{
		uint8_t *dataBuffer = PlMAllocA( chunkSize );
		if ( !ReadChunkData( file, dataBuffer, chunkSize, chunkPosition ) )
		{
			PL_DELETE( dataBuffer );
			return NULL;
		}

		*bufferSize = chunkSize;
		return dataBuffer;
	}

	return NULL;
}

void *apeLoadWav( const char *path, YNCoreAudioWaveFormat *waveFormatEx, unsigned int *bufferSize )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to load wav \"%s\": %s\n", path, PlGetError() );
		return NULL;
	}

	void *dataBuffer = ParseWav( file, waveFormatEx, bufferSize );

	PlCloseFile( file );

	return dataBuffer;
}
