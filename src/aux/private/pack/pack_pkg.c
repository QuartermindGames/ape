// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <plcore/pl_package.h>

#include "../aux_private.h"

#define PKG_MAGIC QM_OS_MAGIC_TO_NUM( 'P', 'K', 'G', '2' )

typedef struct PkgHeader
{
	uint32_t magic;
	uint32_t numFiles;
} PkgHeader;
static const size_t PKG_HEADER_SIZE = sizeof( PkgHeader );

/////////////////////////////////////////////////////////////////
// READ

static QmFsPackage *parse_pkg_file( QmFsFile *file )
{
	PkgHeader header;
	header.magic = qm_fs_file_read_int32( file, false, NULL );
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

	const char *path = qm_fs_file_get_path( file );
	QmFsPackage *package = PlCreatePackageHandle( path, header.numFiles, NULL );
	for ( unsigned int i = 0; i < header.numFiles; ++i )
	{
		QmFsPackageFile *index = &package->files[ i ];

		// read in the filename, it's a sized string...
		uint8_t nameLength = PL_READUINT8( file, NULL );
		qm_file_read( file, index->name, sizeof( char ), nameLength );

		index->name[ nameLength + 1 ] = '\0';

		// file length/size
		index->size = PL_READUINT32( file, false, NULL );
		index->compressedSize = PL_READUINT32( file, false, NULL );

		if ( index->size != index->compressedSize )
			index->compressionType = PL_COMPRESSION_DEFLATE;

		index->offset = qm_fs_file_get_offset( file );

		// now seek to the next file
		if ( !qm_fs_file_seek( file, ( PLFileOffset ) index->compressedSize, QM_FS_SEEK_CUR ) )
		{
			com_warning_( "Failed to seek to the next file within package: %s\n", PlGetError() );
			package->numFiles = ( i + 1 );
			break;
		}
	}

	return package;
}

static QmFsPackage *load_pkg_file( const char *path )
{
	QmFsFile *file = qm_fs_file_open( path, false );
	if ( file == NULL )
		return NULL;

	QmFsPackage *package = parse_pkg_file( file );

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
		qm_os_memory_free( compressedData );
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

	qm_os_memory_free( compressedData );
}
