/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/platform_console.h>

#include "common/common.h"

int logLevelPrint;
int logLevelWarn;

void CommonLibrary_Initialize( void ) {
    logLevelPrint = plAddLogLevel( "common", PL_COLOUR_WHITE, true );
	logLevelWarn = plAddLogLevel( "common/warning", PL_COLOUR_ORANGE, true );
}
