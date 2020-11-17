/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <PL/platform.h>
#include <PL/pl_llist.h>
#include <PL/platform_filesystem.h>

#include "core/format_wld.h"

#define version "0.1"

#define error( ... )       \
	printf( __VA_ARGS__ ); \
	exit( EXIT_FAILURE )

static PLLinkedList *entities;

static const char *SkipWhitespace( const char *in ) {
	while( *in == ' ' && *in != '\0' ) { ++in; }
	return ( *in == ' ' ) ? &in[ 1 ] : in;
}

void Q2W_ParseLine( const char *buffer, unsigned int lineNum ) {
	const char *p = SkipWhitespace( buffer );
	if ( p[ 0 ] == '/' && p[ 1 ] == '/' ) {
		return;
	}
}

void Q2W_ReadMap( const char *path ) {
	PLFile *file = plOpenFile( path, true );
	if ( file == NULL ) {
		error( "Failed to open \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	static unsigned int lineNum = 0;
	const char *p = ( const char * ) plGetFileData( file );
	while ( *p != '\0' ) {
		lineNum++;

		char lineBuffer[ 512 ];
		for ( unsigned int i = 0; i < 512; ++i ) {
			if ( p[ 0 ] == '\r' || p[ 0 ] == '\n' ) {
				break;
			}
		}

		if ( p[ 0 ] == '\r' && p[ 1 ] == '\n' ) {
			p += 2;
			continue;
		} else if ( p[ 0 ] == '\n' ) {
			p++;
			continue;
		}
	}

	plCloseFile( file );
}

int main( int argc, char **argv ) {
#if defined( _WIN32 )
	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );
#endif

	plInitialize( argc, argv );

	printf( "qmap2world v" version " (" __DATE__ " " __TIME__ ")\nCopyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>\n" );

	const char *inputPath = plGetCommandLineArgumentValue( "-map" );
	if ( inputPath == NULL ) {
		printf( "No input path specified, using \"default.map\".\nSpecify using \"-map <path>\" argument.\n" );
		inputPath = "default.map";
	}

	const char *outputPath = plGetCommandLineArgumentValue( "-out" );
	if ( outputPath == NULL ) {
		printf( "No output path specified, using \"default.wld\".\nSpecify using \"-out <path>\" argument.\n" );
		outputPath = "default.wld";
	}

	printf( "INPUT:  %s\n", inputPath );
	printf( "OUTPUT: %s\n", outputPath );

	Q2W_ReadMap( inputPath );

	plShutdown();
}
