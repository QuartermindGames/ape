/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 OldTimes Software
 * ====================================================================*/

#include <PL/platform_filesystem.h>

#include "common/Common.h"

const char *ComFS_GetDataDirectory( void ) {
    static char dataPath[ PL_SYSTEM_MAX_PATH ] = { '\0' };
    if ( dataPath[ 0 ] != '\0' ) {
        return dataPath;
    }

    Message( "Checking for \"" YIN_GLOBAL_WAD "\"\n" );
    if ( !plLocalFileExists( YIN_GLOBAL_WAD ) ) {
        snprintf( dataPath, sizeof( dataPath ), "../../" );
    } else {
        snprintf( dataPath, sizeof( dataPath ), "./" );
    }

    return dataPath;
}
