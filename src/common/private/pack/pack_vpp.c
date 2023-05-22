// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Package loader for VPP format

#include <plcore/pl_package.h>

#include "../common_private.h"

#define BLOCK_SIZE 2048

static const int VPP_MAGIC   = 0x51890ace;
static const int VPP_VERSION = 1;

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
	VppEntry *entries = PL_NEW_( VppEntry, header->numFiles );
	if ( PlReadFile( file, entries, sizeof( VppEntry ), header->numFiles ) == header->numFiles )
	{

	}
	else
	{
		Warning( "Failed to read VPP entries: %s\n", PlGetError() );
	}

	PL_DELETE( entries );

	return package;
}

void cmnPack_RegisterVppInterface_( void )
{
	PlRegisterPackageLoader( "vpp", NULL, ParseVPPFile );
}
