// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Package loader for VPP format

#include <plcore/pl_package.h>

#include "../common_private.h"

// format is optimized for DVD streaming,
// so we'll need to respect that
#define BLOCK_SIZE 2048

static const int32_t VPP_MAGIC = 0x51890ace;
static const int32_t VPP_MAX_VERSION = 3;
static const int32_t VPP_MIN_VERSION = 1;
// 1 - original (Summoner, RF)
// 2 - entry structure changed, introduced compression (RF2 PS2)
// 3 - the same? (Punisher PS2)

typedef struct VppHeader
{
	int32_t magic;
	int32_t version;
	int32_t numFiles;
	int32_t fileSize;
} VppHeader;
PL_STATIC_ASSERT( sizeof( VppHeader ) == 16, "needs to be 16 bytes" );

typedef struct VppEntry
{
	char name[ 60 ];
	int32_t size;
} VppEntry;
PL_STATIC_ASSERT( sizeof( VppEntry ) == 64, "needs to be 64 bytes" );

// this got shrunk down for version 2
typedef struct Vpp2Entry
{
	char name[ 24 ];
	int32_t size;
	int32_t compressedSize;
} Vpp2Entry;
PL_STATIC_ASSERT( sizeof( Vpp2Entry ) == 32, "needs to be 32 bytes" );

static uint32_t calculate_stream_length( uint32_t dataSize )
{
	return ( uint32_t ) ceil( ( double ) dataSize / BLOCK_SIZE ) * BLOCK_SIZE;
}

static PLPackage *parse_vpp_file( PLFile *file )
{
	// below currently doesn't bother or worry about endianess conversion,
	// for simplicity’s sake, but probably worth incorporating at some point

	uint8_t buf[ BLOCK_SIZE ];
	if ( PlReadFile( file, buf, sizeof( uint8_t ), BLOCK_SIZE ) != BLOCK_SIZE )
	{
		return NULL;
	}

	VppHeader *header = ( VppHeader * ) &buf;
	if ( header->magic != VPP_MAGIC )
	{
		Warning( "Invalid magic for VPP: %d != %d\n", header->magic, VPP_MAGIC );
		return NULL;
	}
	else if ( header->version < VPP_MIN_VERSION || header->version > VPP_MAX_VERSION )
	{
		Warning( "Unsupported version for VPP: %d %s %d\n", header->version,
		         ( header->version > VPP_MAX_VERSION ) ? ">" : "<",
		         ( header->version > VPP_MAX_VERSION ) ? VPP_MAX_VERSION : VPP_MIN_VERSION );
		return NULL;
	}

#if 0// todo, load the rest...
	// summoner 2 on PS2 is a special case, so needs an ugly hack...
	bool isSummoner2 = false;
	if ( header->numFiles == 0 && header->fileSize == 0 )
	{
		header->numFiles = *( int32_t * ) ( buf + 60 );
		header->fileSize = *( int32_t * ) ( buf + 64 );
		isSummoner2      = true;
	}
#endif

	if ( header->numFiles == 0 )
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

	uint32_t streamSize = calculate_stream_length( ( header->version == 1 ? sizeof( VppEntry ) : sizeof( Vpp2Entry ) ) * header->numFiles );
	uint8_t *stream = PL_NEW_( uint8_t, streamSize );
	if ( PlReadFile( file, stream, sizeof( uint8_t ), streamSize ) == streamSize )
	{
		PLFileOffset baseOffset = PlGetFileOffset( file );

		package = PlCreatePackageHandle( PlGetFilePath( file ), header->numFiles, NULL );
		for ( uint32_t i = 0; i < package->table_size; ++i )
		{
			if ( header->version == 1 )
			{
				VppEntry *entry = ( ( VppEntry * ) stream ) + i;
				strcpy( package->table[ i ].fileName, entry->name );
				package->table[ i ].fileSize = entry->size;
				package->table[ i ].offset = baseOffset;
				baseOffset += calculate_stream_length( entry->size );
			}
			else
			{
				Vpp2Entry *entry = ( ( Vpp2Entry * ) stream ) + i;
				strcpy( package->table[ i ].fileName, entry->name );
				package->table[ i ].fileSize = entry->size;
				package->table[ i ].compressedSize = entry->compressedSize;
				package->table[ i ].compressionType = ( package->table[ i ].compressedSize != entry->size ) ? PL_COMPRESSION_DEFLATE : PL_COMPRESSION_NONE;
				package->table[ i ].offset = baseOffset;
				baseOffset += calculate_stream_length( entry->compressedSize );
			}
		}
	}
	else
	{
		Warning( "Failed to read in directory table for VPP\n" );
	}

	PL_DELETE( stream );

	return package;
}

void com_pack_vpp_register_( void )
{
	PlRegisterPackageLoader( "vpp", NULL, parse_vpp_file );
}
