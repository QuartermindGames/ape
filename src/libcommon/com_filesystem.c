/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 OldTimes Software
 * ====================================================================*/

#include <plcore/pl_filesystem.h>

#include "common/common.h"

const char *ComFS_GetDataDirectory( void )
{
	static char dataPath[ PL_SYSTEM_MAX_PATH ] = { '\0' };
	if ( dataPath[ 0 ] != '\0' )
	{
		return dataPath;
	}

	Message( "Checking for \"" YIN_GLOBAL_WAD "\"\n" );

	snprintf( dataPath, sizeof( dataPath ), PlLocalFileExists( YIN_GLOBAL_WAD ) ? "./" : "../../" );

	return dataPath;
}
