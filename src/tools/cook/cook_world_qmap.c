// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Process a Quake .map file. Much of this is pulled from my old 'map2world.c' tool.
// Author:  Mark E. Sowden

#include "plcore/pl_linkedlist.h"
#include <plcore/pl_filesystem.h>

#include "qmparse/public/qm_parse.h"

#include "cook.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static constexpr unsigned int MAX_FACE_VERTICES = 16;

typedef struct IdBrushFace
{
	QmMathVector3f x, y, z;

	QmMathVector3f vertices[ MAX_FACE_VERTICES ];
	unsigned int   numVertices;

	char           textureName[ 64 ];
	QmMathVector4f tm[ 2 ];
	float          rotation;
	QmMathVector2f scale;
	QmMathVector3f normal;
	float          distance; /* distance from center */
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
	char          name[ 16 ];
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
	BLOCK_CONTEXT_NONE   = 0U,
	BLOCK_CONTEXT_ENTITY = 1U,
	BLOCK_CONTEXT_BRUSH  = 2U,

	MAX_BLOCK_LEVELS
};
static unsigned int blockLevel = 0;

static void calculate_face_normal( IdBrushFace *face )
{
	QmMathVector3f x, y, z;
	for ( unsigned int i = 0; i < 3; ++i )
	{
		PL_VECTOR_I( x, i ) = PL_VECTOR_I( face->x, i ) - PL_VECTOR_I( face->y, i );
		PL_VECTOR_I( y, i ) = PL_VECTOR_I( face->z, i ) - PL_VECTOR_I( face->y, i );
		PL_VECTOR_I( z, i ) = PL_VECTOR_I( face->y, i );
	}

	face->normal   = qm_math_vector3f_normalize( qm_math_vector3f_cross_product( x, y ) );
	face->distance = qm_math_vector3f_dot_product( z, face->normal );
}

static QmMathVector3f parse_map_vector( const char **p )
{
	if ( **p == '(' )
	{
		( *p )++;
	}

	QmMathVector3f vec;
	qm_parse_vectorfv( p, ( float * ) &vec, 3 );

	if ( **p == ')' )
	{
		( *p )++;
	}

	return vec;
}

static void parse_line( IdMap *map, const char *buffer, unsigned int lineNum )
{
	const char *p = buffer;
	qm_parse_skip_whitespace( &p );
	if ( *p == '/' && *( p + 1 ) == '/' )
	{
		return;
	}

	static IdEntity *currentEntity = nullptr;
	static IdBrush  *currentBrush  = nullptr;

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
				IdEntity *entity   = calloc( 1, sizeof( IdEntity ) );
				entity->brushes    = PlCreateLinkedList();
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

				IdBrush *brush = QM_OS_MEMORY_CALLOC( 1, sizeof( IdBrush ) );
				brush->faces   = PlCreateLinkedList();
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
				currentBrush  = NULL;
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
			IdProperty *property = QM_OS_MEMORY_CALLOC( 1, sizeof( IdProperty ) );
			if ( !qm_parse_enclosed( &p, property->name, sizeof( property->name ) ) )
			{
				ERROR( "Failed to parse enclosed string on line %d!\n", lineNum );
			}

			qm_parse_skip_whitespace( &p );
			if ( !qm_parse_enclosed( &p, property->value, sizeof( property->value ) ) )
			{
				ERROR( "Failed to parse enclosed string on line %d!\n", lineNum );
			}

			DPRINT( " %s %s\n", property->name, property->value );
			if ( pl_strcasecmp( property->name, "wad" ) == 0 ||
			     pl_strcasecmp( property->name, "mapversion" ) == 0 ||
			     pl_strcasecmp( property->name, "_generator" ) == 0 )
			{
				qm_os_memory_free( property );
				break;
			}
			else if ( pl_strcasecmp( property->name, "classname" ) == 0 )
			{
				strncpy( currentEntity->name, property->value, sizeof( currentEntity->name ) );
				qm_os_memory_free( property );
				break;
			}
			PlInsertLinkedListNode( currentEntity->properties, property );
			break;
		}
		case BLOCK_CONTEXT_BRUSH:
		{
			/* read in face */
			IdBrushFace *face = QM_OS_MEMORY_CALLOC( 1, sizeof( IdBrushFace ) );

			face->x = parse_map_vector( &p );
			face->y = parse_map_vector( &p );
			face->z = parse_map_vector( &p );

			calculate_face_normal( face );

			if ( !qm_parse_token( &p, face->textureName, sizeof( face->textureName ) ) )
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
	QmFsFile *file = qm_fs_file_open( path, true );
	if ( file == NULL )
	{
		ERROR( "Failed to open \"%s\"!\nPL: %s\n", path, PlGetError() );
	}

	/* now start reading through every line */
	static unsigned int lineNum = 0;
	const char         *p       = ( const char * ) qm_fs_file_get_data( file );
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
static bool get_intersection( const IdBrushFace *faceA, const IdBrushFace *faceB, QmMathVector3f *p )
{
	float denom = qm_math_vector3f_dot_product( faceA->x, qm_math_vector3f_cross_product( faceA->y, faceA->z ) );
	if ( denom == 0 )
		return false;

	QmMathVector3f c1 = qm_math_vector3f_cross_product( faceA->y, faceA->z );
	QmMathVector3f c2 = qm_math_vector3f_cross_product( faceA->z, faceA->x );
	QmMathVector3f c3 = qm_math_vector3f_cross_product( faceA->x, faceA->y );

	for ( unsigned int i = 0; i < 3; ++i )
	{
		PL_VECTOR_I( p, i ) = -PL_VECTOR_I( faceB->x, i ) * PL_VECTOR_I( c1, i ) -
		                      PL_VECTOR_I( faceB->y, i ) * PL_VECTOR_I( c2, i ) -
		                      PL_VECTOR_I( faceB->z, i ) * PL_VECTOR_I( c3, i ) / denom;
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
