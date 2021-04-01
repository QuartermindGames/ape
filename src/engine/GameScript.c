/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "GameScript.h"

#include "common/Node.h"

#if 0
static TCCState *tccState = NULL;

GameInterface globalGame;

static void ErrorCallback( void *opaque, const char *message ) {
	PrintWarn( "COMPILER: %s\n", message );
}

/* wrapper around tcc_add_file */
static void GS_AddFile( const char *path ) {
	char fullPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( fullPath, sizeof( fullPath ), "%s%s", FS_GetDataDirectory(), path );
	if ( tcc_add_file( tccState, fullPath ) != 0 ) {
        PrintWarn( "Failed to add \"%s\"!\n", fullPath );
	}
}

/* wrapper around tcc_add_include_path */
static void GS_AddIncludePath( const char *path ) {
    char fullPath[ PL_SYSTEM_MAX_PATH ];
    snprintf( fullPath, sizeof( fullPath ), "%s%s", FS_GetDataDirectory(), path );
    if ( tcc_add_include_path( tccState, fullPath ) != 0 ) {
        PrintWarn( "Failed to add \"%s\"!\n", fullPath );
    }
}

static void GS_LoadProjectFile( const char *path ) {
    NLNode *root = NL_LoadFile( path, "project" );
    if ( root == NULL ) {
        PrintWarn( "Failed to load project file, \"%s\"!\n", path );
		return;
    }

    NLNode *child = NL_GetFirstChild( root );
	while ( child != NULL ) {
		const char *name = NL_GetName( child );
		if ( strcmp( name, "path" ) == 0 ) {
			GS_AddFile( NL_GetString( child ) );
		} else if ( strcmp( name, "includePath" ) == 0 ) {
            GS_AddIncludePath( NL_GetString( child ) );
		}

		child = NL_GetNextChild( child );
	}

	/* cleanup */
	NL_DestroyNode( root );
}

/**
 * Setup the global game interface.
 */
static void GS_SetupGameInterface( void ) {
	memset( &globalGame, 0, sizeof( GameInterface ) );

#define ADD_VALIDATE( PTR, FUNC ) PTR = tcc_get_symbol( tccState, ( FUNC ) ); \
	if ( ( PTR ) == NULL ) { PrintError( "Failed to fetch function \"%s\"!\n", ( FUNC ) ); }

	ADD_VALIDATE( globalGame.GameInit, "GameInit" );
}

void GS_CompileScripts( void ) {
	tccState = tcc_new();
	if ( tccState == NULL ) {
		PrintError( "Failed to create TCC context!\n" );
	}

	tcc_set_error_func( tccState, NULL, ErrorCallback );

	/* load in the project script */
#if 0
    CVar( "game.projectPath", gameProjectPath );
	GS_LoadProjectFile( gameProjectPath->s_value );
#else
	GS_AddIncludePath( "scripts/" );
    GS_AddFile( "scripts/Game.c" );
#endif

	tcc_set_output_type( tccState, TCC_OUTPUT_MEMORY );

	tcc_relocate( tccState, TCC_RELOCATE_AUTO );

    GS_SetupGameInterface();

	globalGame.GameInit();
}

void GS_Execute( void ) {

}

void GS_Cleanup( void ) {
	if ( tccState != NULL ) {
		tcc_delete( tccState );
		tccState = NULL;
	}
}
#endif
