/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <PL/platform.h>
#include <PL/pl_llist.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_math.h>

#include "core/wld.h"

#define VERSION "0.1"
#define dprint( ... )   printf( __VA_ARGS__ )
#define error( ... )    printf( __VA_ARGS__ ); exit( EXIT_FAILURE )

typedef struct IdBrushFace {
	PLVector3 x, y, z;
	char textureName[ 64 ];
	PLVector4 tm[ 2 ];
	float rotation;
	PLVector2 scale;
} IdBrushFace;

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

	MAX_BLOCK_LEVELS
};
static unsigned int blockLevel = 0;

static void SkipWhitespace( const char **p ) {
	const char *pp = *p;
	while ( *pp == ' ' && *pp != '\0' ) { ++pp; }
	if ( *pp == ' ' ) ++pp;
	*p = pp;
}

static const char *ParseEnclosedString( const char **p, char *dest, size_t size ) {
	const char *pp = *p;
	if ( *pp == '\"' ) { pp++; }
	size_t i = 0;
	while( *pp != '\0' && *pp != '\"') {
		if ( !( i >= size ) ) {
			dest[ i++ ] = *pp;
		}
		pp++;
	}
	if ( *pp == '\"' ) { pp++; }
	*p = pp;
	return dest;
}

static const char *ParseToken( const char **p, char *dest, size_t size ) {
	const char *pp = *p;
	memset( dest, 0, size );
	SkipWhitespace( &pp );
	size_t i = 0;
	while( *pp != '\0' && *pp != ' ') {
		if ( !( i >= size ) ) {
			dest[ i++ ] = *pp;
		}
		pp++;
	}

	if ( *pp == ' ' ) { pp++; }
	*p = pp;
	return dest;
}

static int ParseInteger( const char **p ) {
	const char *i = *p;

	char num[ 32 ];
	if ( !ParseToken( &i, num, sizeof( num ) ) ) {
		printf( "Failed to parse integer!\n" );
		return 0;
	}

	*p = i;
	return strtol( num, NULL, 10 );
}

static float ParseFloat( const char **p ) {
	const char *i = *p;

	char num[ 32 ];
	if ( !ParseToken( &i, num, sizeof( num ) ) ) {
		printf( "Failed to parse float!\n" );
		return 0;
	}

	*p = i;
	return strtof( num, NULL );
}

static PLVector3 ParseVector( const char **p ) {
	const char *i = *p;
	SkipWhitespace( &i );
	if ( *i == '(' ) { i++; }
	float x = ParseInteger( &i );
	float y = ParseInteger( &i );
	float z = ParseInteger( &i );
	if ( *i == ')' ) { i++; }
	*p = i;
	return PLVector3( x, y, z );
}

void Q2W_ParseLine( IdMap *map, const char *buffer, unsigned int lineNum ) {
	const char *p = buffer;
	SkipWhitespace( &p );
	if ( *p == '/' && *( p + 1 ) == '/' ) {
		return;
	}

	static IdEntity *currentEntity = NULL;
	static IdBrush *currentBrush = NULL;

	if ( *p == '{' ) {
		blockLevel++;
		/*dprint( "up: %d\n", blockLevel );*/
		if ( blockLevel >= MAX_BLOCK_LEVELS ) {
			error( "Invalid opening brace on line %d!\n", lineNum );
		}

		switch( blockLevel ) {
			case BLOCK_CONTEXT_ENTITY: {
				dprint( "entity\n" );
				IdEntity *entity = calloc( 1, sizeof( IdEntity ) );
				entity->brushes = plCreateLinkedList();
				entity->properties = plCreateLinkedList();
				plInsertLinkedListNode( map->entities, entity );
				currentEntity = entity;
				break;
			}
			case BLOCK_CONTEXT_BRUSH: {
				dprint( "brush\n" );
				/* will probably never happen, but better safe than sorry! */
				if ( currentEntity == NULL ) {
					error( "Hit a brush without a valid entity!\n" );
				}

				IdBrush *brush = calloc( 1, sizeof( IdBrush ) );
				brush->faces = plCreateLinkedList();
				plInsertLinkedListNode( currentEntity->brushes, brush );
				currentBrush = brush;
				break;
			}
			default:
				dprint( "none\n" );
				break;
		}

		return;
	} else if ( *p == '}' ) {
		/* throw an error if we're already outside a block */
		if ( blockLevel == 0 ) {
			error( "Invalid closing brace on line %d!\n", lineNum );
		}

		blockLevel--;
		/*dprint( "down: %d\n", blockLevel );*/

		switch( blockLevel ) {
			case BLOCK_CONTEXT_NONE:
				/*dprint( "none\n" );*/
				currentEntity = NULL;
				currentBrush = NULL;
				break;
			case BLOCK_CONTEXT_ENTITY:
				/*dprint( "entity\n" );*/
				currentBrush = NULL;
				break;
			default:
				/*dprint( "brush\n" );*/
				break;
		}

		return;
	}

	switch( blockLevel ) {
		case BLOCK_CONTEXT_ENTITY: {
			/* read in property */
			WLDActorProperty *property = calloc( 1, sizeof( WLDActorProperty ) );
			if ( !ParseEnclosedString( &p, property->name, sizeof( property->name ) ) ) {
				error( "Failed to parse enclosed string on line %d!\n", lineNum );
			}
			SkipWhitespace( &p );
			if ( !ParseEnclosedString( &p, property->value, sizeof( property->value ) ) ) {
				error( "Failed to parse enclosed string on line %d!\n", lineNum );
			}
			dprint( " %s %s\n", property->name, property->value );
			plInsertLinkedListNode( currentEntity->properties, property );
			break;
		}
		case BLOCK_CONTEXT_BRUSH: {
			/* read in face */
			IdBrushFace *face = calloc( 1, sizeof( IdBrushFace ) );
			face->x = ParseVector( &p );
			dprint( "%s ", plPrintVector3( &face->x, pl_int_var ) );
			face->y = ParseVector( &p );
			dprint( "%s ", plPrintVector3( &face->y, pl_int_var ) );
			face->z = ParseVector( &p );
			dprint( "%s ", plPrintVector3( &face->z, pl_int_var ) );
			if ( !ParseToken( &p, face->textureName, sizeof( face->textureName ) ) ) {
				error( "Failed to fetch texture name on line %d!\n", lineNum );
			}
			dprint( "%s\n", face->textureName );
			plInsertLinkedListNode( currentBrush->faces, face );
			break;
		}
		default:
			break;
	}
}

void Q2W_ReadMap( IdMap *map, const char *path ) {
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
		memset( lineBuffer, 0, sizeof( lineBuffer ) );
		for ( unsigned int i = 0; i < sizeof( lineBuffer ) - 1; ++i ) {
			if ( *p == '\0' || *p == '\r' && *( p + 1 ) == '\n' || *p == '\n' ) {
				break;
			}

			lineBuffer[ i ] = *p++;
		}

		Q2W_ParseLine( map, lineBuffer, lineNum );

		if ( *p == '\r' && *( p + 1 ) == '\n' ) { p += 2; }
		else if ( *p == '\n' ) { p++; }
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

static void Q2W_WriteChildNodes( FILE *file, IdMap *map ) {
	PLLinkedListNode *node = plGetRootNode( map->textures );
	while ( node != NULL ) {
		static bool isWorldspawn = true;
		if ( isWorldspawn ) {
			/* skip worldspawn */
			isWorldspawn = false;
		} else {
			IdEntity *entity = plGetLinkedListNodeUserData( node );



			PLLinkedListNode *propertyNode = plGetRootNode( entity->properties );
		}

		node = plGetNextLinkedListNode( node );
	}
}

static void Q2W_WriteNodes( FILE *file, IdMap *map ) {
	/* for now just write one global sector */
	uint32_t numSectors = 1;
	fwrite( &numSectors, sizeof( uint32_t ), 1, file );

	for ( unsigned int i = 0; i < numSectors; ++i ) {
		WLDVector mins = { -99999, -99999, -99999 };
		WLDVector maxs = { 99999, 99999, 99999 };
		fwrite( &mins, sizeof( WLDVector ), 1, file );
		fwrite( &maxs, sizeof( WLDVector ), 1, file );

		/* ignoring worldspawn, hence -1 */
		uint32_t numChildren = plGetNumLinkedListNodes( map->entities ) - 1;
		fwrite( &numChildren, sizeof( uint32_t ), 1, file );

		Q2W_WriteChildNodes( file, map );
	}
}

static void Q2W_WriteWld( IdMap *map, const char *path ) {
	FILE *file = fopen( path, "wb" );
	if ( file == NULL ) {
		error( "Failed to write file, \"%s\"!\n", path );
	}

	Q2W_WriteHeader( file, map );
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
		printf( "No output path specified, using \"default " WORLD_EXTENSION "\".\nSpecify using \"-out <path>\" argument.\n" );
		outputPath = "default" WORLD_EXTENSION;
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
