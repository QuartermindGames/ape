// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Post processing handler
// Author:  Mark E. Sowden

#include "model_rfc.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static const unsigned int RFC_MAGIC = 0x87128712;

// Traditionally, we supported a range but this didn't work out so well,
// so instead we'll do this, so if another version is ever found,
// it'll get flagged as unsupported instead until we can verify it
static const unsigned int RFC_SUPPORTED_VERSIONS[] = {
        0, // Red Faction (PS2 Demo)
        1, // Red Faction (PS2)
        10,// Red Faction 2 (PS2 Demo)
};
static const unsigned int RFC_NUM_SUPPORTED_VERSIONS = PL_ARRAY_ELEMENTS( RFC_SUPPORTED_VERSIONS );

/////////////////////////////////////////////////////////////////////////////////////
// Public

AclModelRfc *acl_model_rfc_parse_file( PLFile *file )
{
	unsigned int magic = PL_READUINT32( file, false, NULL );
	if ( magic != RFC_MAGIC )
	{
		PRINT_WARNING( "Not an RFC file!\n" );
		return NULL;
	}

	unsigned int version = PL_READUINT32( file, false, NULL );
	unsigned int i;
	for ( i = 0; i < RFC_NUM_SUPPORTED_VERSIONS; ++i )
	{
		if ( version != RFC_SUPPORTED_VERSIONS[ i ] )
			continue;

		break;
	}

	if ( i == RFC_NUM_SUPPORTED_VERSIONS )
	{
		PRINT_WARNING( "Unsupported RFC version (%u)!\n", i );
		return NULL;
	}
}

AclModelRfc *acl_model_rfc_load_file( const char *filename )
{
	PLFile *file = PlOpenFile( filename, false );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to load RFC model: %s\n", PlGetError() );
		return NULL;
	}

	AclModelRfc *rfc = acl_model_rfc_parse_file( file );

	PlCloseFile( file );
	return rfc;
}
