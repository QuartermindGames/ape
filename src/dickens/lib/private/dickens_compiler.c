/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_filesystem.h>
#include <plcore/pl_linkedlist.h>

#include "dickens_private.h"

unsigned int LOG_LEVEL_DEFAULT;
unsigned int LOG_LEVEL_WARNING;
unsigned int LOG_LEVEL_ERROR;

static PLPath ycOutputPath = "out.yex";

static bool assemble_callback( const char *parm )
{
	if ( parm == NULL )
	{
		Warning( "No parameter provided!\n" );
		return false;
	}

	PLFile *file = PlOpenFile( parm, true );
	if ( file == NULL )
	{
		PlLogMessage( LOG_LEVEL_ERROR, "Failed to open file: %s\n", PlGetError() );
		return false;
	}

	PLPath outPath;
	snprintf( outPath, sizeof( PLPath ), "%s.obj", PlGetFileName( parm ) );

	const char *fileBuf = ( const char * ) PlGetFileData( file );
	size_t fileLength = PlGetFileSize( file );
	bool status = dk_assemble_from_buffer( fileBuf, fileLength, "out.obj" );

	PlCloseFile( file );

	if ( !status )
	{
		return false;
	}

	return true;
}

static void ParseLexerOutput( PLLinkedList *tokenList )
{
	PLLinkedListNode *node = PlGetFirstNode( tokenList );
	while ( node != NULL )
	{
		const DkLexerToken *lexerToken = PlGetLinkedListNodeUserData( node );

		node = PlGetNextLinkedListNode( node );
	}
}

static void BuildFile( const char *path )
{
	Print( "Building \"%s\"\n", path );

	/* load the file */
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		Error( "Failed to open \"%s\": %s\n", path, PlGetError() );
	}

	/* copy it into a null-terminated buffer */
	size_t fileSize = PlGetFileSize( file );
	char *buf = PlMAllocA( fileSize + 1 );
	memcpy( buf, PlGetFileData( file ), fileSize );
	PlCloseFile( file );

	/* create our list and pass it into the lexer */
	DkLexer *lexer;
	if ( ( lexer = dk_generate_token_list( NULL, buf, path ) ) == NULL )
		Error( "Lexer failed to generate token table!\nSee logs for more information.\n" );

	ParseLexerOutput( lexer->tokens );

	/* generate path for module output */
	PLPath outPath = "out/";
	strcat( outPath, PlGetFileName( path ) );
	PlStripExtension( outPath, sizeof( outPath ), PlGetFileName( path ) );
	strcat( outPath, ".yb" );
}

static bool begin_build_project_callback( const char *parm )
{
	if ( parm == NULL )
	{
		Warning( "No parameter provided!\n" );
		return false;
	}

	NLNode *root = NL_LoadFile( parm, "project" );
	if ( root == NULL )
	{
		Warning( "Failed to load specified project: %s\n", NL_GetErrorMessage() );
		return false;
	}

	if ( !PlCreateDirectory( "out" ) )
	{
		Warning( "Failed to create output directory: %s\n", PlGetError() );
		return false;
	}

	const char *outputPath = NL_GetStrByName( root, "output", NULL );
	if ( outputPath != NULL )
	{
		snprintf( ycOutputPath, sizeof( ycOutputPath ), "%s", outputPath );
	}

	NLNode *fileList = NL_GetChildByName( root, "files" );
	if ( fileList != NULL )
	{
		NLNode *child = NL_GetFirstChild( fileList );
		while ( child != NULL )
		{
			PLPath path;
			NL_GetStr( child, path, sizeof( path ) );

			BuildFile( path );

			child = NL_GetNextChild( child );
		}
	}

	NL_DestroyNode( root );

	return true;
}

#if 0
int main( int argc, char **argv )
{
#	if defined( _WIN32 )
	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );
#	endif

	PlInitialize( argc, argv );

	PlSetupLogOutput( DICKENS_LOG_PATH );
	LOG_LEVEL_DEFAULT = PlAddLogLevel( "yc", PL_COLOUR_GREEN, true );
	LOG_LEVEL_WARNING = PlAddLogLevel( "yc/warning", PL_COLOUR_YELLOW, true );
	LOG_LEVEL_ERROR = PlAddLogLevel( "yc/error", PL_COLOUR_RED, true );

	printf( "YANG Compiler Suite v%d.%d.%d\n"
	        "Written by Mark E Sowden for Project Yin\n"
	        "========================================\n",
	        DICKENS_VERSION_MAJOR,
	        DICKENS_VERSION_MINOR,
	        DICKENS_VERSION_PATCH );

	BeginBuildProjectCallback( "project.node" );

	return 0;

	typedef struct Arg
	{
		const char *parm;
		bool ( *Callback )( const char *parm );
		unsigned int numArguments;
		const char  *description;
	} Arg;
	Arg arguments[] =
			{
					{ "-asm", AssembleCallback, 1, "Assembles the given block of assembly." },
					{ "-p", BeginBuildProjectCallback, 1, "Parse the given project file and compile." },
			};
	static unsigned int numSupportedArguments = PL_ARRAY_ELEMENTS( arguments );

	for ( int i = 0; i < argc; ++i )
	{
		for ( int j = 0; j < numSupportedArguments; ++j )
		{
			if ( pl_strcasecmp( argv[ i ], arguments[ j ].parm ) != 0 )
			{
				continue;
			}

			const char *parm = ( i + 1 < argc ) ? argv[ i + 1 ] : NULL;
			if ( arguments[ i ].Callback != NULL && !arguments[ i ].Callback( parm ) )
			{
				Error( "Failed on command line argument, \"%s\". Aborting.\n", arguments[ j ].parm );
			}
		}
	}

	return EXIT_SUCCESS;
}
#endif
