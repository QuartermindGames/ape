// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Process a Quake .map file. Much of this is pulled from my old 'map2world.c' tool.
// Author:  Mark E. Sowden

#include "plcore/pl_linkedlist.h"
#include "plcore/pl_parse.h"

#include "cook.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static constexpr unsigned int MAX_FACE_VERTICES = 16;

typedef struct IdBrushFace
{
	PLVector3 x, y, z;

	PLVector3 vertices[ MAX_FACE_VERTICES ];
	unsigned int numVertices;

	char textureName[ 64 ];
	PLVector4 tm[ 2 ];
	float rotation;
	PLVector2 scale;
	PLVector3 normal;
	float distance; /* distance from center */
} IdBrushFace;

typedef struct IdBrush
{
	PLLinkedList *faces;
} IdBrush;

typedef struct IdProperty
{
	char name[ 32 ];
	char value[ 256 ];
} IdProperty;

typedef struct IdEntity
{
	char name[ 16 ];
	PLLinkedList *properties;
	PLLinkedList *brushes;
} IdEntity;

typedef char IdTexture[ 64 ];

typedef struct IdMap
{
	PLLinkedList *entities;
	PLLinkedList *textures;
} IdMap;

enum
{
	BLOCK_CONTEXT_NONE = 0U,
	BLOCK_CONTEXT_ENTITY = 1U,
	BLOCK_CONTEXT_BRUSH = 2U,

	MAX_BLOCK_LEVELS
};
static unsigned int blockLevel = 0;

static void calculate_face_normal( IdBrushFace *face )
{
	PLVector3 x, y, z;
	for ( unsigned int i = 0; i < 3; ++i )
	{
		PlVectorIndex( x, i ) = PlVectorIndex( face->x, i ) - PlVectorIndex( face->y, i );
		PlVectorIndex( y, i ) = PlVectorIndex( face->z, i ) - PlVectorIndex( face->y, i );
		PlVectorIndex( z, i ) = PlVectorIndex( face->y, i );
	}

	face->normal = PlNormalizeVector3( PlVector3CrossProduct( x, y ) );
	face->distance = PlVector3DotProduct( z, face->normal );
}

static void parse_line( IdMap *map, const char *buffer, unsigned int lineNum )
{
	const char *p = buffer;
	PlSkipWhitespace( &p );
	if ( *p == '/' && *( p + 1 ) == '/' )
	{
		return;
	}

	static IdEntity *currentEntity = NULL;
	static IdBrush *currentBrush = NULL;

	if ( *p == '{' )
	{
		blockLevel++;
		/*dprint( "up: %d\n", blockLevel );*/
		if ( blockLevel >= MAX_BLOCK_LEVELS )
		{
			ERROR( "Invalid opening brace on line %d!\n", lineNum );
		}

		switch ( blockLevel )
		{
			case BLOCK_CONTEXT_ENTITY:
			{
				DPRINT( "entity\n" );
				IdEntity *entity = calloc( 1, sizeof( IdEntity ) );
				entity->brushes = PlCreateLinkedList();
				entity->properties = PlCreateLinkedList();
				PlInsertLinkedListNode( map->entities, entity );
				currentEntity = entity;
				break;
			}
			case BLOCK_CONTEXT_BRUSH:
			{
				DPRINT( "brush\n" );
				/* will probably never happen, but better safe than sorry! */
				if ( currentEntity == NULL )
				{
					ERROR( "Hit a brush without a valid entity!\n" );
				}

				IdBrush *brush = PlCAllocA( 1, sizeof( IdBrush ) );
				brush->faces = PlCreateLinkedList();
				PlInsertLinkedListNode( currentEntity->brushes, brush );
				currentBrush = brush;
				break;
			}
			default:
				DPRINT( "none\n" );
				break;
		}

		return;
	}
	else if ( *p == '}' )
	{
		/* throw an error if we're already outside a block */
		if ( blockLevel == 0 )
		{
			ERROR( "Invalid closing brace on line %d!\n", lineNum );
		}

		blockLevel--;
		/*dprint( "down: %d\n", blockLevel );*/

		switch ( blockLevel )
		{
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

	switch ( blockLevel )
	{
		case BLOCK_CONTEXT_ENTITY:
		{
			/* read in property */
			IdProperty *property = PlCAllocA( 1, sizeof( IdProperty ) );
			if ( !PlParseEnclosedString( &p, property->name, sizeof( property->name ) ) )
			{
				ERROR( "Failed to parse enclosed string on line %d!\n", lineNum );
			}

			PlSkipWhitespace( &p );
			if ( !PlParseEnclosedString( &p, property->value, sizeof( property->value ) ) )
			{
				ERROR( "Failed to parse enclosed string on line %d!\n", lineNum );
			}

			DPRINT( " %s %s\n", property->name, property->value );
			if ( pl_strcasecmp( property->name, "wad" ) == 0 ||
			     pl_strcasecmp( property->name, "mapversion" ) == 0 ||
			     pl_strcasecmp( property->name, "_generator" ) == 0 )
			{
				PlFree( property );
				break;
			}
			else if ( pl_strcasecmp( property->name, "classname" ) == 0 )
			{
				strncpy( currentEntity->name, property->value, sizeof( currentEntity->name ) );
				PlFree( property );
				break;
			}
			PlInsertLinkedListNode( currentEntity->properties, property );
			break;
		}
		case BLOCK_CONTEXT_BRUSH:
		{
			/* read in face */
			bool status;
			IdBrushFace *face = PlCAllocA( 1, sizeof( IdBrushFace ) );
			face->x = PlParseVector( &p, &status );
			DPRINT( "%s ", PlPrintVector3( &face->x, PL_VAR_I32 ) );
			face->y = PlParseVector( &p, &status );
			DPRINT( "%s ", PlPrintVector3( &face->y, PL_VAR_I32 ) );
			face->z = PlParseVector( &p, &status );
			DPRINT( "%s ", PlPrintVector3( &face->z, PL_VAR_I32 ) );
			if ( !status )
			{
				ERROR( "Failed to parse vector on line %d!\n", lineNum );
			}

			calculate_face_normal( face );

			if ( !PlParseToken( &p, face->textureName, sizeof( face->textureName ) ) )
			{
				ERROR( "Failed to fetch texture name on line %d!\n", lineNum );
			}

			DPRINT( "%s\n", face->textureName );

			PlInsertLinkedListNode( currentBrush->faces, face );
			break;
		}
		default:
			break;
	}
}

static void read_map( IdMap *map, const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		ERROR( "Failed to open \"%s\"!\nPL: %s\n", path, PlGetError() );
	}

	/* now start reading through every line */
	static unsigned int lineNum = 0;
	const char *p = ( const char * ) PlGetFileData( file );
	while ( *p != '\0' )
	{
		lineNum++;

		char lineBuffer[ 512 ] = {};
		for ( unsigned int i = 0; i < sizeof( lineBuffer ) - 1; ++i )
		{
			if ( *p == '\0' || ( *p == '\r' && *( p + 1 ) == '\n' ) || *p == '\n' )
			{
				break;
			}

			lineBuffer[ i ] = *p++;
		}

		parse_line( map, lineBuffer, lineNum );

		if ( *p == '\r' && *( p + 1 ) == '\n' )
		{
			p += 2;
		}
		else if ( *p == '\n' )
		{
			p++;
		}
	}

	PlCloseFile( file );
}

/* Based upon the documentation found here.
 * https://github.com/stefanha/map-files/blob/master/MAPFiles.pdf
 * */
static bool get_intersection( const IdBrushFace *faceA, const IdBrushFace *faceB, PLVector3 *p )
{
	float denom = PlVector3DotProduct( faceA->x, PlVector3CrossProduct( faceA->y, faceA->z ) );
	if ( denom == 0 )
		return false;

	PLVector3 c1 = PlVector3CrossProduct( faceA->y, faceA->z );
	PLVector3 c2 = PlVector3CrossProduct( faceA->z, faceA->x );
	PLVector3 c3 = PlVector3CrossProduct( faceA->x, faceA->y );

	for ( unsigned int i = 0; i < 3; ++i )
	{
		PlVectorIndex( p, i ) = -PlVectorIndex( faceB->x, i ) * PlVectorIndex( c1, i ) -
		                        PlVectorIndex( faceB->y, i ) * PlVectorIndex( c2, i ) -
		                        PlVectorIndex( faceB->z, i ) * PlVectorIndex( c3, i ) / denom;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void cook_world_process_qmap( const char *path )
{
	IdMap map = {};
	read_map( &map, path );
}
