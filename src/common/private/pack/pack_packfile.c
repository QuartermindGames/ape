// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Package loader for RF2 package format

#include <plcore/pl_package.h>

#include "../common_private.h"

static PLPackage *ParseTocGroupFile( PLFile *file )
{
	char tmp[ 128 ];

	if ( PlReadString( file, tmp, sizeof( tmp ) ) == NULL )
	{
		Warning( "Failed to read in toc name!\n" );
		return NULL;
	}

	if ( PlReadString( file, tmp, sizeof( tmp ) ) == NULL )
	{
		Warning( "Failed to read in toc path!\n" );
		return NULL;
	}


}

void cmnRegisterPackageInterface_( void )
{
	PlRegisterPackageLoader( "toc_group", NULL, ParseTocGroupFile );
}
