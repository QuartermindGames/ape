/* ======================================================================
 * Yin C/VM Suite
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================== */

#include <PL/platform_filesystem.h>

#include "ycvm.h"
#include "yc.h"

unsigned int LOG_LEVEL_DEFAULT;
unsigned int LOG_LEVEL_WARNING;
unsigned int LOG_LEVEL_ERROR;

static const char *specials[] = {
        '$', '=', '.',
};

static const char *reservedWords[] = {
        "and",

        "byte",
        "float",
        "" };

int main( int argc, char **argv ) {
	plInitialize( argc, argv );

	plSetupLogOutput( YC_LOG_PATH );
	LOG_LEVEL_DEFAULT = plAddLogLevel( "yc", PL_COLOUR_GREEN, true );
	LOG_LEVEL_WARNING = plAddLogLevel( "yc/warning", PL_COLOUR_ORANGE, true );
	LOG_LEVEL_ERROR = plAddLogLevel( "yc/error", PL_COLOUR_RED, true );

	Print( "Yin Compiler\n"
	       "Written by Mark E Sowden for Project Yin\n"
	       "===================================\n" );

	if ( argc < 2 ) {
		Print( "Usage:\n"
		       " <project_path> [-<option> ...]" );
	}

	PLFile *file = plOpenFile( argv[ 1 ], true );
	if ( file == NULL ) {
		Error( "Failed to open \"%s\"!\n", argv[ 1 ] );
	}

	const char *buf = plGetFileData( file );
}
