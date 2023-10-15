// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl.h>
#include <plcore/pl_linkedlist.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_math.h>
#include <plcore/pl_parse.h>

#include <plgraphics/plg_polygon.h>

#include "yin/core_world.h"
#include "yin/node.h"

#define VERSION "0.1"
#if 0
#	define dprint( ... ) printf( __VA_ARGS__ )
#else
#	define dprint( ... )
#endif
#define error( ... )           \
	{                          \
		printf( __VA_ARGS__ ); \
		exit( EXIT_FAILURE );  \
	}

#define MAX_FACE_VERTICES 16

typedef struct IdBrushFace {
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

typedef struct IdBrush {
	PLLinkedList *faces;
} IdBrush;

typedef struct IdProperty {
	char name[ 32 ];
	char value[ 256 ];
} IdProperty;

typedef struct IdEntity {
	char name[ 16 ];
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

static void CalculateFaceNormal( IdBrushFace *face ) {
	PLVector3 x, y, z;
	for ( unsigned int i = 0; i < 3; ++i ) {
		PlVectorIndex( x, i ) = PlVectorIndex( face->x, i ) - PlVectorIndex( face->y, i );
		PlVectorIndex( y, i ) = PlVectorIndex( face->z, i ) - PlVectorIndex( face->y, i );
		PlVectorIndex( z, i ) = PlVectorIndex( face->y, i );
	}

	face->normal = PlNormalizeVector3( PlVector3CrossProduct( x, y ) );
	face->distance = PlVector3DotProduct( z, face->normal );
}

static void ParseLine( IdMap *map, const char *buffer, unsigned int lineNum ) {
	const char *p = buffer;
	PlSkipWhitespace( &p );
	if ( *p == '/' && *( p + 1 ) == '/' )
		return;

	static IdEntity *currentEntity = NULL;
	static IdBrush *currentBrush = NULL;

	if ( *p == '{' ) {
		blockLevel++;
		/*dprint( "up: %d\n", blockLevel );*/
		if ( blockLevel >= MAX_BLOCK_LEVELS )
			error( "Invalid opening brace on line %d!\n", lineNum );

		switch ( blockLevel ) {
			case BLOCK_CONTEXT_ENTITY: {
				dprint( "entity\n" );
				IdEntity *entity = calloc( 1, sizeof( IdEntity ) );
				entity->brushes = PlCreateLinkedList();
				entity->properties = PlCreateLinkedList();
				PlInsertLinkedListNode( map->entities, entity );
				currentEntity = entity;
				break;
			}
			case BLOCK_CONTEXT_BRUSH: {
				dprint( "brush\n" );
				/* will probably never happen, but better safe than sorry! */
				if ( currentEntity == NULL ) {
					error( "Hit a brush without a valid entity!\n" );
				}

				IdBrush *brush = PlCAllocA( 1, sizeof( IdBrush ) );
				brush->faces = PlCreateLinkedList();
				PlInsertLinkedListNode( currentEntity->brushes, brush );
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
		if ( blockLevel == 0 )
			error( "Invalid closing brace on line %d!\n", lineNum );

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
			IdProperty *property = PlCAllocA( 1, sizeof( IdProperty ) );
			if ( !PlParseEnclosedString( &p, property->name, sizeof( property->name ) ) )
				error( "Failed to parse enclosed string on line %d!\n", lineNum );

			PlSkipWhitespace( &p );
			if ( !PlParseEnclosedString( &p, property->value, sizeof( property->value ) ) )
				error( "Failed to parse enclosed string on line %d!\n", lineNum );

			dprint( " %s %s\n", property->name, property->value );
			if ( pl_strcasecmp( property->name, "wad" ) == 0 ||
			     pl_strcasecmp( property->name, "mapversion" ) == 0 ||
			     pl_strcasecmp( property->name, "_generator" ) == 0 ) {
				PlFree( property );
				break;
			} else if ( pl_strcasecmp( property->name, "classname" ) == 0 ) {
				strncpy( currentEntity->name, property->value, sizeof( currentEntity->name ) );
				PlFree( property );
				break;
			}
			PlInsertLinkedListNode( currentEntity->properties, property );
			break;
		}
		case BLOCK_CONTEXT_BRUSH: {
			/* read in face */
			bool status;
			IdBrushFace *face = PlCAllocA( 1, sizeof( IdBrushFace ) );
			face->x = PlParseVector( &p, &status );
			dprint( "%s ", PlPrintVector3( &face->x, pl_int_var ) );
			face->y = PlParseVector( &p, &status );
			dprint( "%s ", PlPrintVector3( &face->y, pl_int_var ) );
			face->z = PlParseVector( &p, &status );
			dprint( "%s ", PlPrintVector3( &face->z, pl_int_var ) );
			if ( !status )
				error( "Failed to parse vector on line %d!\n", lineNum );

			CalculateFaceNormal( face );

			if ( !PlParseToken( &p, face->textureName, sizeof( face->textureName ) ) )
				error( "Failed to fetch texture name on line %d!\n", lineNum );

			dprint( "%s\n", face->textureName );

			PlInsertLinkedListNode( currentBrush->faces, face );
			break;
		}
		default:
			break;
	}
}

static void ReadMap( IdMap *map, const char *path ) {
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
		error( "Failed to open \"%s\"!\nPL: %s\n", path, PlGetError() );

	/* now start reading through every line */
	static unsigned int lineNum = 0;
	const char *p = ( const char * ) PlGetFileData( file );
	while ( *p != '\0' ) {
		lineNum++;

		char lineBuffer[ 512 ];
		memset( lineBuffer, 0, sizeof( lineBuffer ) );
		for ( unsigned int i = 0; i < sizeof( lineBuffer ) - 1; ++i ) {
			if ( *p == '\0' || ( *p == '\r' && *( p + 1 ) == '\n' ) || *p == '\n' )
				break;

			lineBuffer[ i ] = *p++;
		}

		ParseLine( map, lineBuffer, lineNum );

		if ( *p == '\r' && *( p + 1 ) == '\n' )
			p += 2;
		else if ( *p == '\n' )
			p++;
	}

	PlCloseFile( file );
}

/*****************************************************************************************/

static NdBranch *globalProperties;

/* Based upon the documentation found here.
 * https://github.com/stefanha/map-files/blob/master/MAPFiles.pdf
 * */
static bool GetIntersection( const IdBrushFace *faceA, const IdBrushFace *faceB, PLVector3 *p ) {
	float denom = PlVector3DotProduct( faceA->x, PlVector3CrossProduct( faceA->y, faceA->z ) );
	if ( denom == 0 )
		return false;

	PLVector3 c1 = PlVector3CrossProduct( faceA->y, faceA->z );
	PLVector3 c2 = PlVector3CrossProduct( faceA->z, faceA->x );
	PLVector3 c3 = PlVector3CrossProduct( faceA->x, faceA->y );

	for ( unsigned int i = 0; i < 3; ++i ) {
		PlVectorIndex( p, i ) = -PlVectorIndex( faceB->x, i ) * PlVectorIndex( c1, i ) -
		                        PlVectorIndex( faceB->y, i ) * PlVectorIndex( c2, i ) -
		                        PlVectorIndex( faceB->z, i ) * PlVectorIndex( c3, i ) / denom;
	}

	return true;
}

static void WriteBrush( NdBranch *root, NdBranch *materialsNode, IdBrush *brush ) {
	// Move all the faces into an array
	unsigned int numFaces;
	IdBrushFace **faces = ( IdBrushFace ** ) PlArrayFromLinkedList( brush->faces, &numFaces );
	// And now, the horrible part
	for ( unsigned int i = 0; i < numFaces - 3; ++i ) {
		for ( unsigned int j = 0; j < numFaces - 2; ++j ) {
			for ( unsigned int k = 0; k < numFaces - 1; ++k ) {
				if ( i != ( j == k ) )
					continue;

				PLVector3 p;
				if ( !GetIntersection( faces[ j ], faces[ k ], &p ) )
					continue;

				faces[ i ]->vertices[ faces[ i ]->numVertices++ ] = p;
				faces[ j ]->vertices[ faces[ j ]->numVertices++ ] = p;
				faces[ k ]->vertices[ faces[ k ]->numVertices++ ] = p;
			}
		}
	}

	NdBranch *facesNode = ndPushBackObjectArray( root, "faces" );
	for ( unsigned int i = 0; i < numFaces; ++i ) {
		if ( pl_strcasecmp( "skip", faces[ i ]->textureName ) == 0 )
			continue;

		NdBranch *faceNode = ndPushBackObject( facesNode, "face" );
		NdBranch *verticesNode = ndPushBackObjectArray( faceNode, "vertices" );
		for ( unsigned int j = 0; j < 3; ++j ) {
			PLGVertex vertex;
			PL_ZERO_( vertex );

#if 0
			vertex.position = faces[ i ]->x;
			NL_DS_SerializeVertex( verticesNode, NULL, &vertex );

			vertex.position = faces[ i ]->y;
			NL_DS_SerializeVertex( verticesNode, NULL, &vertex );

			vertex.position = faces[ i ]->z;
			NL_DS_SerializeVertex( verticesNode, NULL, &vertex );
#endif
		}
	}

	PL_DELETE( faces );
}

#if 0
static void WriteEntity( IdEntity *entity, FILE *file )
{
	WriteNodeHeader( entity->name, WLD_NODE_ENTITY, PlGetNumLinkedListNodes( entity->brushes ), file );
	WriteProperties( entity->properties, file );

	PLLinkedListNode *node = PlGetFirstNode( entity->brushes );
	while ( node != NULL )
	{
		IdBrush *brush = PlGetLinkedListNodeUserData( node );
		WriteBrush( brush, file );
		node = PlGetNextLinkedListNode( node );
	}

	PlDestroyLinkedList( entity->brushes );
}
#endif

static void WriteGlobalProperties( void *userData, bool *breakEarly ) {
	IdProperty *idProperty = userData;
	ndPushBackString( globalProperties, idProperty->name, idProperty->value );
}

static void WriteWorldMesh( NdBranch *root, PLLinkedList *brushes ) {
	NdBranch *worldMeshNode = ndPushBackObject( root, "worldMesh" );

	NdBranch *materials = ndPushBackStringArray( worldMeshNode, "materials", NULL, 0 );
	NdBranch *vertices = ndPushBackF32Array( worldMeshNode, "vertices", NULL, 0 );
	NdBranch *faces = ndPushBackObjectArray( worldMeshNode, "faces" );

	PLLinkedListNode *brushNode = PlGetFirstNode( brushes );
	while ( brushNode != NULL ) {
		IdBrush *brush = PlGetLinkedListNodeUserData( brushNode );
		PLLinkedListNode *faceNode = PlGetFirstNode( brush->faces );
		while ( faceNode != NULL ) {
			IdBrushFace *face = PlGetLinkedListNodeUserData( faceNode );
			NdBranch *materialNode = ndGetFirstChild( materials );
			while ( materialNode != NULL ) {
				char materialPath[ 64 ];
				ndGetStr( materialNode, materialPath, sizeof( materialPath ) );
				if ( strcmp( materialPath, face->textureName ) == 0 ) {
					break;
				}

				materialNode = ndGetNextChild( materialNode );
			}

			if ( materialNode == NULL ) {
				ndPushBackString( materials, NULL, face->textureName );
			}

			faceNode = PlGetNextLinkedListNode( faceNode );
		}

		brushNode = PlGetNextLinkedListNode( brushNode );
	}
}

static void WriteNodes( NdBranch *root, IdMap *map ) {
	IdEntity *worldSpawn = PlGetLinkedListNodeUserData( PlGetFirstNode( map->entities ) );
	if ( worldSpawn == NULL ) {
		error( "Failed to fetch worldspawn!\n" );
	}

	/* first write out the global properties */
	globalProperties = ndPushBackObject( root, "properties" );
	PlIterateLinkedList( worldSpawn->properties, WriteGlobalProperties, true );

	char id[ 64 ];
	PlGenerateUniqueIdentifier( id, sizeof( id ) );
	NdBranch *meshNode = ndPushBackObject( ndPushBackObjectArray( root, "meshes" ), NULL );
	{
		ndPushBackString( meshNode, "id", id );

		NdBranch *materialsNode = ndPushBackObjectArray( meshNode, "materials" );

		/* now write out all the brushes */
		PLLinkedListNode *node = PlGetFirstNode( worldSpawn->brushes );
		while ( node != NULL ) {
			IdBrush *brush = PlGetLinkedListNodeUserData( node );
			WriteBrush( meshNode, materialsNode, brush );
			node = PlGetNextLinkedListNode( node );
		}
	}

	/* and all the entities */
#if 0
	node = PlGetNextLinkedListNode( PlGetFirstNode( map->entities ) );
	while ( node != NULL )
	{
		IdEntity *entity = PlGetLinkedListNodeUserData( node );
		WriteEntity( entity, file );
		node = PlGetNextLinkedListNode( node );
	}
#endif

	/* write out the default room */
	NdBranch *defaultSector = ndPushBackObject( ndPushBackObjectArray( root, "sectors" ), NULL );
	ndPushBackString( defaultSector, "id", "main.room" );
	ndPushBackString( defaultSector, "meshId", id );
}

static void WriteWorld( IdMap *map, const char *path ) {
	NdBranch *root = ndPushBackObject( NULL, "world" );
	ndPushBackI32( root, "version", APE_WORLD_VERSION );

	WriteNodes( root, map );

	ndWriteFile( path, root, ND_FILE_BINARY );
}

int main( int argc, char **argv ) {
	PlInitialize( argc, argv );

	printf( "m2w v" VERSION " (" __DATE__ " " __TIME__ ")\n"
	        "Copyright (C) 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>\n" );

	const char *inputPath = PlGetCommandLineArgumentValue( "-map" );
	if ( inputPath == NULL ) {
		printf( "No input path specified, using \"default.map\".\nSpecify using \"-map <path>\" argument.\n" );
		inputPath = "default.map";
	}

	const char *outputPath = PlGetCommandLineArgumentValue( "-out" );
	if ( outputPath == NULL ) {
		printf( "No output path specified, using default.\nSpecify using \"-out <path>\" argument.\n" );

		char *tmpPath = PlMAllocA( strlen( PlGetFileName( inputPath ) ) + strlen( "." APE_WORLD_EXTENSION ) );
		const char *c = strrchr( inputPath, '.' );
		strncpy( tmpPath, inputPath, c - inputPath );
		strcat( tmpPath, "." APE_WORLD_EXTENSION );

		outputPath = tmpPath;
	}

	printf( "INPUT:  %s\n", inputPath );
	printf( "OUTPUT: %s\n", outputPath );

	IdMap *map = PlMAllocA( sizeof( IdMap ) );
	map->entities = PlCreateLinkedList();
	map->textures = PlCreateLinkedList();

	ReadMap( map, inputPath );
	WriteWorld( map, outputPath );

	PlShutdown();
}
