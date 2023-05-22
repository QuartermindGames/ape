// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Package loader for VPP format

#include <plcore/pl_package.h>

#include "../common_private.h"

// format is optimized for DVD streaming,
// so we'll need to respect that
#define BLOCK_SIZE 2048

static const int VPP_MAGIC   = 0x51890ace;
static const int VPP_VERSION = 1;

static uint32_t CalculateStreamLength( uint32_t dataSize )
{
	return ( uint32_t ) ceil( ( double ) dataSize / BLOCK_SIZE ) * BLOCK_SIZE;
}

static PLPackage *ParseVPPFile( PLFile *file )
{
	// below currently doesn't bother or worry about endianess conversion,
	// for simplicity’s sake, but probably worth incorporating at some point

	uint8_t buf[ BLOCK_SIZE ];
	if ( PlReadFile( file, buf, sizeof( uint8_t ), BLOCK_SIZE ) != BLOCK_SIZE )
	{
		return NULL;
	}

	typedef struct VppHeader
	{
		int32_t magic;
		int32_t version;
		int32_t numFiles;
		int32_t fileSize;
	} VppHeader;
	VppHeader *header = ( VppHeader * ) &buf;
	if ( header->magic != VPP_MAGIC )
	{
		Warning( "Invalid magic for VPP: %d != %d\n", header->magic, VPP_MAGIC );
		return NULL;
	}
	else if ( header->version > VPP_VERSION )
	{
		Warning( "Unsupported version for VPP: %d > %d\n", header->version, VPP_VERSION );
		return NULL;
	}
	else if ( header->numFiles == 0 )
	{
		Warning( "Empty VPP\n" );
		return NULL;
	}
	else if ( header->fileSize != PlGetFileSize( file ) )
	{
		Warning( "Unexpected file size for VPP\n" );
		return NULL;
	}

	PLPackage *package = NULL;

	typedef struct VppEntry
	{
		char name[ 60 ];
		int32_t size;
	} VppEntry;
	PL_STATIC_ASSERT( sizeof( VppEntry ) == 64, "needs to be 64 bytes" );

	uint32_t streamSize = CalculateStreamLength( sizeof( VppEntry ) * header->numFiles );
	uint8_t *stream     = PL_NEW_( uint8_t, streamSize );
	if ( PlReadFile( file, stream, sizeof( uint8_t ), streamSize ) == streamSize )
	{
		PLFileOffset baseOffset = PlGetFileOffset( file );

		package = PlCreatePackageHandle( PlGetFilePath( file ), header->numFiles, NULL );
		for ( unsigned int i = 0; i < package->table_size; ++i )
		{
			VppEntry *entry = ( ( VppEntry * ) stream ) + i;
			strcpy( package->table[ i ].fileName, entry->name );
			package->table[ i ].fileSize = entry->size;
			package->table[ i ].offset   = baseOffset;
			baseOffset += CalculateStreamLength( entry->size );
		}
	}
	else
	{
		Warning( "Failed to read in directory table for VPP\n" );
	}

	PL_DELETE( stream );

	return package;
}

void cmnPack_RegisterVppInterface_( void )
{
	PlRegisterPackageLoader( "vpp", NULL, ParseVPPFile );
}
