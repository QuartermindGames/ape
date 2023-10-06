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
	

	return NULL;
}
