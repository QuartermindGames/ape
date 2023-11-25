// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: RFC loader
// Author:  Mark E. Sowden

#include "../common_private.h"

#include <plcore/pl_package.h>

/////////////////////////////////////////////////////////////////////////////////////
// Private

static const unsigned int KEG_MAGIC = PL_MAGIC_TO_NUM( 'G', 'E', 'K', 'V' );// VKEG

static const unsigned int KEG_VERSION_RF2 = 6;
static const unsigned int KEG_VERSION_PUN = 7;
// The Punisher on PC is 1, for whatever reason...

static const unsigned int KEG_VERSION_MIN = KEG_VERSION_RF2;
static const unsigned int KEG_VERSION_MAX = KEG_VERSION_PUN;

typedef struct KegVbmHeader
{
	uint16_t width;
	uint16_t height;
	uint8_t bitmapType;
	uint8_t paletteType;
	uint8_t flags;
	uint8_t numFrames;
	uint8_t framerate;
	uint8_t numMips;
	int16_t filterValue;
	char filename[ 48 ];
	uint32_t dataOffset;
} KegVbmHeader;

/////////////////////////////////////////////////////////////////////////////////////
// Public

PLPackage *com_pack_keg_parse_file_( PLFile *file )
{
	unsigned int magic = PL_READUINT32( file, false, NULL );
	if ( magic != KEG_MAGIC )
	{
		Warning( "Invalid magic for KEG: %d != %d\n", magic, KEG_MAGIC );
		return NULL;
	}

	unsigned int version = PL_READUINT32( file, false, NULL );
	if ( version < KEG_VERSION_MIN || version > KEG_VERSION_MAX )
	{
		Warning( "Invalid version for KEG (%u)!\n", version );
		return NULL;
	}

	unsigned int headerSize = PL_READUINT32( file, false, NULL );
	unsigned int dataSize = PL_READUINT32( file, false, NULL );
	unsigned int numBitmaps = PL_READUINT32( file, false, NULL );
	unsigned int numFlags = PL_READUINT32( file, false, NULL );
	unsigned int frameCount = PL_READUINT32( file, false, NULL );
	unsigned int totalEntries = PL_READUINT32( file, false, NULL );
	unsigned int alignValue = PL_READUINT32( file, false, NULL );
}
