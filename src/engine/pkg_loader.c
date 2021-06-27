/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "pkg_loader.h"
#include "miniz.h"

uint8_t *Pkg_OpenFile( PLFile *file, PLPackageIndex *index )
{
	if ( !PlFileSeek( file, ( signed ) index->offset, PL_SEEK_SET ) )
		return NULL;

	uint8_t *data = globalSystem.MAlloc( index->compressedSize, true );
	if ( PlReadFile( file, data, index->compressedSize, 1 ) != 1 )
	{
		globalSystem.Free( data );
		return NULL;
	}

	if ( index->compressionType == PL_COMPRESSION_ZLIB )
	{
		/* go ahead and decompress it */
		uint8_t *     uncompressedData = globalSystem.MAlloc( index->fileSize, true );
		unsigned long uncompressedLength;
		int           status = mz_uncompress( uncompressedData, &uncompressedLength, data, ( mz_ulong ) index->compressedSize );

		/* don't need this anymore! */
		globalSystem.Free( data );
		data = uncompressedData;

		if ( status != MZ_OK )
		{
			globalSystem.Free( uncompressedData );
			PrintWarn( "Failed to decompress \"%s\" from package \"%s\"!\n", index->fileName, PlGetFilePath( file ) );
			return NULL;
		}
	}

	return data;
}

PLPackage *Pkg_LoadPackage( const char *path )
{
	PLFile *filePtr = PlOpenFile( path, false );
	if ( filePtr == NULL )
	{
		PrintWarn( "Failed to open package \"%s\"!\nPL: %s\n", path, PlGetError() );
		return NULL;
	}

	/* read in the header */
	char identifier[ 4 ];
	if ( PlReadFile( filePtr, identifier, 1, sizeof( identifier ) ) != sizeof( identifier ) )
	{
		PlCloseFile( filePtr );

		PrintWarn( "Failed to read in identifier for \"%s\"!\nPL: %s\n", path, PlGetError() );
		return NULL;
	}

	if ( !( identifier[ 0 ] == 'P' && identifier[ 1 ] == 'K' && identifier[ 2 ] == 'G' && identifier[ 3 ] == '2' ) )
		PrintError( "Invalid package header, \"%s\", expected \"PKG2\"!\n", identifier );

	bool     status;
	uint32_t numFiles = PlReadInt32( filePtr, false, &status );
	if ( !status )
		PrintError( "Failed to read in the number of files within the \"%s\" package!\nPL: %s\n", path, PlGetError() );

	PLPackage *package = PlCreatePackageHandle( path, numFiles, Pkg_OpenFile );
	for ( unsigned int i = 0; i < numFiles; ++i )
	{
		PLPackageIndex *index = &package->table[ i ];

		/* read in the filename, it's a sized string... */
		uint8_t nameLength = PlReadInt8( filePtr, &status );
		if ( PlReadFile( filePtr, index->fileName, sizeof( char ), nameLength ) != nameLength )
			PrintError( "Failed to read in filename within the \"%s\" package!\nPL: %s\n", path, PlGetError() );

		index->fileName[ nameLength + 1 ] = '\0';

		/* file length/size */
		index->fileSize       = PlReadInt32( filePtr, false, &status );
		index->compressedSize = PlReadInt32( filePtr, false, &status );
		if ( !status )
			PrintError( "Failed to read in the file sizes for \"%s\" within the \"%s\" package!\nPL: %s\n", index->fileName, path, PlGetError() );

		if ( index->fileSize != index->compressedSize )
			index->compressionType = PL_COMPRESSION_ZLIB;

		index->offset = PlGetFileOffset( filePtr );

		/* now seek to the next file */
		if ( !PlFileSeek( filePtr, index->compressedSize, PL_SEEK_CUR ) )
			PrintError( "Failed to seek to the next file within the \"%s\" package!\nPL: %s\n", path, PlGetError() );

		/* PrintMsg( " Registered %s\n", index->fileName ); */
	}

	PlCloseFile( filePtr );

	return package;
}
