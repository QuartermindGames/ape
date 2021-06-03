/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_console.h>

#include "common/common.h"

int logLevelPrint;
int logLevelWarn;

void CommonLibrary_Initialize( void )
{
	logLevelPrint = PlAddLogLevel( "common", PL_COLOUR_WHITE, true );
	logLevelWarn  = PlAddLogLevel( "common/warning", PL_COLOUR_ORANGE, true );

	Message( "Common Library initialized\n" );
}
