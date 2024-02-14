// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_package.h>

#include "../common_private.h"

#define PKG_MAGIC PL_MAGIC_TO_NUM( 'P', 'K', 'G', '2' )

typedef struct PkgHeader
{
	uint32_t magic;
	uint32_t numFiles;
} PkgHeader;
static const size_t PKG_HEADER_SIZE = sizeof( PkgHeader );

/////////////////////////////////////////////////////////////////
// READ

static PLPackage *parse_pkg_file( PLFile *file )
{
	PkgHeader header;
	header.magic = PlReadInt32( file, false, NULL );
	if ( header.magic != PKG_MAGIC )
	{
		com_warning_( "Unexpected magic for pkg: %d\n", header.magic );
		return NULL;
	}

	header.numFiles = PL_READUINT32( file, false, NULL );
	if ( header.numFiles == 0 )
	{
		com_warning_( "Empty package!\n" );
		return NULL;
	}

	const char *path = PlGetFilePath( file );
	PLPackage *package = PlCreatePackageHandle( path, header.numFiles, NULL );
	for ( unsigned int i = 0; i < header.numFiles; ++i )
	{
		PLPackageIndex *index = &package->table[ i ];

		// read in the filename, it's a sized string...
		uint8_t nameLength = PL_READUINT8( file, NULL );
		PlReadFile( file, index->fileName, sizeof( char ), nameLength );

		index->fileName[ nameLength + 1 ] = '\0';

		// file length/size
		index->fileSize = PL_READUINT32( file, false, NULL );
		index->compressedSize = PL_READUINT32( file, false, NULL );

		if ( index->fileSize != index->compressedSize )
			index->compressionType = PL_COMPRESSION_DEFLATE;

		index->offset = PlGetFileOffset( file );

		// now seek to the next file
		if ( !PlFileSeek( file, ( PLFileOffset ) index->compressedSize, PL_SEEK_CUR ) )
		{
			com_warning_( "Failed to seek to the next file within package: %s\n", PlGetError() );
			package->table_size = ( i + 1 );
			break;
		}
	}

	return package;
}

static PLPackage *load_pkg_file( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
		return NULL;

	PLPackage *package = parse_pkg_file( file );

	PlCloseFile( file );

	return package;
}

void com_pack_pkg_register_( void )
{
	PlRegisterPackageLoader( "pkg", load_pkg_file, parse_pkg_file );
}

/////////////////////////////////////////////////////////////////
// WRITE

void com_pkg_write_header( FILE *pack, unsigned int numFiles )
{
	fseek( pack, 0, SEEK_SET );
	fwrite( &( PkgHeader ){ .magic = PKG_MAGIC,
	                        .numFiles = numFiles },
	        PKG_HEADER_SIZE, 1, pack );
}

void com_pkg_add_data( FILE *pack, const char *path, const void *buf, size_t size )
{
	uint8_t nameLength = ( uint8_t ) strlen( path );
	fwrite( &nameLength, sizeof( uint8_t ), 1, pack );
	fwrite( path, sizeof( char ), nameLength, pack );
	fwrite( &size, sizeof( uint32_t ), 1, pack );

	size_t compressedSize;
	void *compressedData = PlCompress_Deflate( buf, size, &compressedSize );
	if ( compressedData == NULL )
	{
		compressedSize = size;
		com_warning_( "Failed to compress data: %s\n", PlGetError() );
	}
	else if ( compressedSize >= size )
	{
		PL_DELETE( compressedData );
		compressedData = NULL;
		compressedSize = size;
	}
	else
	{
		size = compressedSize;
		buf = compressedData;
	}

	// this is our compressed size, if it's the same as the decompressed size,
	// it's assumed the file isn't compressed
	fwrite( &size, sizeof( uint32_t ), 1, pack );

	fwrite( buf, sizeof( char ), size, pack );

	PL_DELETE( compressedData );
}
