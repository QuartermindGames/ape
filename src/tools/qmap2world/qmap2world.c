/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <PL/platform.h>
#include <PL/pl_llist.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_math.h>

#include "core/format_wld.h"

#define VERSION "0.1"
#define error( ... )       \
	printf( __VA_ARGS__ ); \
	exit( EXIT_FAILURE )

typedef struct IdFace {
	PLVector3 x, y, z;
	char textureName[ 64 ];
	float rotation;
	float uScale, vScale;
} IdFace;

typedef struct IdBrush {
	PLLinkedList *faces;
} IdBrush;

typedef struct IdEntity {
	PLLinkedList *properties;
	PLLinkedList *brushes;
} IdEntity;

typedef char IdTexture[ 64 ];

typedef struct IdMap {
	PLLinkedList *entities;
	PLLinkedList *textures;
} IdMap;

enum {
	BLOCK_CONTEXT_NONE = 0U,
	BLOCK_CONTEXT_ENTITY = 1U,
	BLOCK_CONTEXT_BRUSH = 2U,
};
static unsigned int blockLevel = 0;

static const char *SkipWhitespace( const char *in ) {
	while ( *in == ' ' && *in != '\0' ) { ++in; }
	return ( *in == ' ' ) ? &in[ 1 ] : in;
}

void Q2W_ParseLine( const char *buffer, unsigned int lineNum ) {
	const char *p = SkipWhitespace( buffer );
	if ( *p == '/' && *( p + 1 ) == '/' ) {
		return;
	}

	if ( *p == '{' ) {
		blockLevel++;
		return;
	} else if ( *p == '}' ) {
		/* throw an error if we're already outside a block */
		if ( blockLevel == 0 ) {
			error( "Invalid closing brace on line %d!\n", lineNum );
		}

		blockLevel--;
		return;
	}
}

void Q2W_ReadMap( const char *path ) {
	PLFile *file = plOpenFile( path, true );
	if ( file == NULL ) {
		error( "Failed to open \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	/* now start reading through every line */
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

static void Q2W_WriteHeader( FILE *file, IdMap *map ) {
	/* write out the header */
	WLDHeader header = {
			.magic = WORLD_MAGIC,
			.version = WORLD_VERSION,
			.createdTime = ( unsigned long ) time( NULL ),
			.modifiedTime = ( unsigned long ) time( NULL ),
	};
	plGetUserName( header.author, sizeof( header.author ) );
	fwrite( &header, sizeof( WLDHeader ), 1, file );
}

static void Q2W_WriteMaterials( FILE *file, IdMap *map ) {
	/* materials */
	fprintf( file, "materials %d\n", plGetNumLinkedListNodes( map->textures ) );
	PLLinkedListNode *node = plGetRootNode( map->textures );
	while ( node != NULL ) {
		const char *textureName = plGetLinkedListNodeUserData( node );
		fprintf( file, "%s\n", textureName );
		node = plGetNextLinkedListNode( node );
	}
}

static void Q2W_WriteNodes( FILE *file, IdMap *map ) {
	/* nodes */
	fprintf( file, "sg_nodes %d\n", plGetNumLinkedListNodes( map->entities ) - 1 );
	PLLinkedListNode *node = plGetRootNode( map->textures );
	while ( node != NULL ) {
		/* skip worldspawn */
		static bool isWorldspawn = true;
		if ( isWorldspawn ) {
			isWorldspawn = false;
		} else {
			const IdEntity *entity = plGetLinkedListNodeUserData( node );



			PLLinkedListNode *propertyNode = plGetRootNode( entity->properties );
		}

		node = plGetNextLinkedListNode( node );
	}
}

static void Q2W_WriteWld( IdMap *map, const char *path ) {
	FILE *file = fopen( path, "wb" );
	if ( file == NULL ) {
		error( "Failed to write file, \"%s\"!\n", path );
	}

	Q2W_WriteHeader( file, map );
	Q2W_WriteMaterials( file, map );
	Q2W_WriteNodes( file, map );
}

int main( int argc, char **argv ) {
#if defined( _WIN32 )
	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );
#endif

	plInitialize( argc, argv );

	printf( "qmap2world v" VERSION " (" __DATE__ " " __TIME__ ")\nCopyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>\n" );

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

	IdMap *map = calloc( 1, sizeof( IdMap ) );
	map->entities = plCreateLinkedList();
	map->textures = plCreateLinkedList();

	Q2W_ReadMap( map, inputPath );
	Q2W_WriteWld( map, outputPath );

	plShutdown();
}
