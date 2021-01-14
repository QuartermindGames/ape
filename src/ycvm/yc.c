/* ======================================================================
 * Yin C/VM Suite
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================== */

#include <PL/platform_filesystem.h>

#include "ycvm.h"
#include "yc.h"

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
	plSetupLogLevel( LOG_LEVEL_DEFAULT, NULL, PL_COLOUR_GREEN, true );
	plSetupLogLevel( LOG_LEVEL_WARNING, "warning", PL_COLOUR_ORANGE, true );
	plSetupLogLevel( LOG_LEVEL_ERROR, "error", PL_COLOUR_RED, true );

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
