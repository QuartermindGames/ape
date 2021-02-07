/* ======================================================================
 * QMAP2WORLD, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/platform.h>
#include <PL/pl_llist.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_math.h>
#include <PL/pl_parse.h>

#include "common/wld.h"

#define VERSION "0.1"
#define dprint( ... ) printf( __VA_ARGS__ )
#define error( ... )       \
	printf( __VA_ARGS__ ); \
	exit( EXIT_FAILURE )

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
	WldTag name;
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

void Q2W_ParseLine( IdMap *map, const char *buffer, unsigned int lineNum ) {
	const char *p = buffer;
	plSkipWhitespace( &p );
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

		switch ( blockLevel ) {
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

		switch ( blockLevel ) {
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

	switch ( blockLevel ) {
		case BLOCK_CONTEXT_ENTITY: {
			/* read in property */
			WldProperty *property = calloc( 1, sizeof( WldProperty ) );
			if ( !plParseEnclosedString( &p, property->name, sizeof( property->name ) ) ) {
				error( "Failed to parse enclosed string on line %d!\n", lineNum );
			}
			plSkipWhitespace( &p );
			if ( !plParseEnclosedString( &p, property->value, sizeof( property->value ) ) ) {
				error( "Failed to parse enclosed string on line %d!\n", lineNum );
			}
			dprint( " %s %s\n", property->name, property->value );
			if ( pl_strcasecmp( property->name, "wad" ) == 0 ||
			     pl_strcasecmp( property->name, "mapversion" ) == 0 ||
			     pl_strcasecmp( property->name, "_generator" ) == 0 ) {
				free( property );
				break;
			} else if ( pl_strcasecmp( property->name, "classname" ) == 0 ) {
				strncpy( currentEntity->name, property->value, sizeof( currentEntity->name ) );
				free( property );
				break;
			}
			plInsertLinkedListNode( currentEntity->properties, property );
			break;
		}
		case BLOCK_CONTEXT_BRUSH: {
			/* read in face */
			IdBrushFace *face = calloc( 1, sizeof( IdBrushFace ) );
			face->x = plParseVector( &p );
			dprint( "%s ", plPrintVector3( &face->x, pl_int_var ) );
			face->y = plParseVector( &p );
			dprint( "%s ", plPrintVector3( &face->y, pl_int_var ) );
			face->z = plParseVector( &p );
			dprint( "%s ", plPrintVector3( &face->z, pl_int_var ) );
			if ( !plParseToken( &p, face->textureName, sizeof( face->textureName ) ) ) {
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

		if ( *p == '\r' && *( p + 1 ) == '\n' ) {
			p += 2;
		} else if ( *p == '\n' ) {
			p++;
		}
	}

	plCloseFile( file );
}

/*****************************************************************************************/

static void WriteSizedString( FILE *file, const char *msg ) {
	size_t length = strlen( msg );
	if ( length >= UINT8_MAX ) {
		printf( "String \"%s\" is too long for WLD tag, truncating!\n", msg );
		length = UINT8_MAX;
	}
	fwrite( &length, sizeof( uint8_t ), 1, file );
	fwrite( msg, sizeof( char ), length, file );
}

static void WriteHeader( FILE *file, IdMap *map ) {
	fwrite( WORLD_MAGIC, sizeof( char ), 4, file );
	fwrite( &( int ){ WORLD_VERSION }, sizeof( uint32_t ), 1, file );
}

static void WriteNodeHeader( const char *tag, WldNodeType type, uint32_t numChildren, FILE *file ) {
	WriteSizedString( file, tag );
	fwrite( &type, sizeof( uint8_t ), 1, file );
	fwrite( &numChildren, sizeof( uint32_t ), 1, file );
}

static void WriteProperties( PLLinkedList *propertyList, FILE *file ) {
	uint16_t numProperties = plGetNumLinkedListNodes( propertyList );
	fwrite( &numProperties, sizeof( uint16_t ), 1, file );
	if ( numProperties == 0 ) {
		return;
	}

	PLLinkedListNode *root = plGetRootNode( propertyList );
	while ( root != NULL ) {
		WldProperty *property = plGetLinkedListNodeUserData( root );
		WriteSizedString( file, property->name );
		WriteSizedString( file, property->value );
		root = plGetNextLinkedListNode( root );
	}
}

/* https://github.com/stefanha/map-files/blob/master/MAPFiles.pdf */
static bool GetIntersection( ) {

}

static void WriteBrush( IdBrush *brush, FILE *file ) {
	static unsigned int numBrush = 0;

	char tbuf[ WORLD_MAX_TAG ];
	snprintf( tbuf, sizeof( tbuf ), "brush%d", numBrush );
	WriteNodeHeader( tbuf, WLD_NODE_BRUSH, 0, file );

	/* move it all into an array */
	unsigned int numFaces = plGetNumLinkedListNodes( brush->faces );
	IdBrushFace *faces = calloc( numFaces, sizeof( IdBrushFace ) );
	{
		unsigned int i = 0;
		PLLinkedListNode *node = plGetRootNode( brush->faces );
		while ( node != NULL ) {
			IdBrushFace *face = plGetLinkedListNodeUserData( node );
			faces[ i ] = *face;
			free( face );

			node = plGetNextLinkedListNode( node );
		}
		plDestroyLinkedList( brush->faces );
	}

	for ( unsigned int i = 0; i < numFaces - 3; ++i ) {
		for ( unsigned int j = i; j < numFaces - 2; ++j ) {
			for ( unsigned int k = j; k < numFaces - 1; ++k ) {
				if ( !( i != j != k ) ) {
					continue;
				}

				bool isLegal = true;
				for ( unsigned int m = 0; m < numFaces - 1; ++m ) {

				}

				if ( isLegal ) {

				}
			}
		}
	}

	WldVector *vertices = calloc( numFaces * 16, sizeof( WldVector ) );

	numBrush++;
}

static void WriteEntity( IdEntity *entity, FILE *file ) {
	WriteNodeHeader( entity->name, WLD_NODE_ENTITY, plGetNumLinkedListNodes( entity->brushes ), file );
	WriteProperties( entity->properties, file );

	PLLinkedListNode *node = plGetRootNode( entity->brushes );
	while ( node != NULL ) {
		IdBrush *brush = plGetLinkedListNodeUserData( node );
		WriteBrush( brush, file );
		node = plGetNextLinkedListNode( node );
	}
	plDestroyLinkedList( entity->brushes );
}

static void WriteNodes( FILE *file, IdMap *map ) {
	IdEntity *worldSpawn = plGetLinkedListNodeUserData( plGetRootNode( map->entities ) );
	if ( worldSpawn == NULL ) {
		error( "Failed to fetch worldspawn!\n" );
	}

	/* first write out all of the world properties */
	WriteProperties( worldSpawn->properties, file );

	/* write out the default room */
	fwrite( &( int ){ 1 }, sizeof( uint32_t ), 1, file );
	unsigned int numChildren = 0;
	numChildren += plGetNumLinkedListNodes( worldSpawn->brushes );
	numChildren += plGetNumLinkedListNodes( map->entities ) - 1; /* ignore worldspawn */
	WriteNodeHeader( "room0", WLD_NODE_SECTOR, numChildren, file );

	PLLinkedListNode *node;
	/* now write out all the brushes */
	node = plGetRootNode( worldSpawn->brushes );
	while ( node != NULL ) {
		IdBrush *brush = plGetLinkedListNodeUserData( node );
		WriteBrush( brush, file );
		node = plGetNextLinkedListNode( node );
	}
	/* and all the entities */
	node = plGetNextLinkedListNode( plGetRootNode( map->entities ) );
	while ( node != NULL ) {
		IdEntity *entity = plGetLinkedListNodeUserData( node );
		WriteEntity( entity, file );
		node = plGetNextLinkedListNode( node );
	}
}

static void Q2W_WriteWld( IdMap *map, const char *path ) {
	FILE *file = fopen( path, "wb" );
	if ( file == NULL ) {
		error( "Failed to write file, \"%s\"!\n", path );
	}

	WriteHeader( file, map );
	WriteNodes( file, map );

	fclose( file );
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
