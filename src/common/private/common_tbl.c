// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Helper functions for parsing tbl formatted files.
// Author:  Mark E. Sowden

#include "common_private.h"
#include "common/common_tbl.h"
#include "plcore/pl_parse.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// Public

const char *com_tbl_get_token( const char **p, char *buf, size_t size )
{
}

bool com_tbl_validate_type( const char **p, const char *type )
{
	while ( *( *p ) != '#' )
		PlSkipLine( p );

	if ( *( *p ) != '#' )
		return false;

	( *p )++;

	char buf[ 32 ];
	if ( PlParseLine( p, buf, sizeof( buf ) ) == NULL )
		return false;

	return ( strcmp( buf, type ) == 0 );
}
