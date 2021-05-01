/* ======================================================================
 * Yin C/VM Suite
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================== */

#include <plcore/pl_filesystem.h>

#include "ycvm.h"
#include "yc.h"

static char specials[] = {
        '$',
        '=',
        '.',
};

static const char *reservedWords[] = {
        "and",

        "byte",
        "float",
        "" };

unsigned int
        LOG_LEVEL_DEFAULT,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_ERROR;

int main( int argc, char **argv ) {
	PlInitialize( argc, argv );

	PlSetupLogOutput( YC_LOG_PATH );
	LOG_LEVEL_DEFAULT = PlAddLogLevel( NULL, PL_COLOUR_GREEN, true );
	LOG_LEVEL_WARNING = PlAddLogLevel( "yin/warning", PL_COLOUR_ORANGE, true );
	LOG_LEVEL_ERROR = PlAddLogLevel( "yin/error", PL_COLOUR_RED, true );

	Print( "Yin Compiler\n"
	       "Written by Mark E Sowden for Project Yin\n"
	       "===================================\n" );

	if ( argc < 2 ) {
		Print( "Usage:\n"
		       " <project_path> [-<option> ...]" );
	}

	PLFile *file = PlOpenFile( argv[ 1 ], true );
	if ( file == NULL ) {
		Error( "Failed to open \"%s\"!\n", argv[ 1 ] );
	}

	const char *buf = PlGetFileData( file );
}
