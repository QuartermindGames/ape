/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl_parse.h>
#include <plmodel/plm.h>

#include "yin.h"

enum
{
	PLY_VAR_INVALID,
	PLY_VAR_FLOAT,
	PLY_VAR_DOUBLE,
	PLY_VAR_UCHAR,
	PLY_VAR_CHAR,
	PLY_VAR_USHORT,
	PLY_VAR_SHORT,
	PLY_VAR_UINT,
	PLY_VAR_INT,
};

typedef struct PLYVariable
{
	unsigned int type;
	union
	{
		float          f;
		double         f64;
		double         d;
		unsigned char  uc;
		char           c;
		unsigned short us;
		short          s;
		unsigned int   ui;
		int            i;
	};
} PLYVariable;

typedef struct PLYVertexDescription
{
	PLYVariable x, y, z;
	PLYVariable nx, ny, nz;
	PLYVariable s, t;
	PLYVariable r, g, b;
} PLYVertexDescription;

typedef struct PLYFaceDescription
{
	PLYVariable numVertices;
	PLYVariable indices[ 32 ];
} PLYFaceDescription;

typedef struct PLYHeader
{
	PLYVertexDescription vertexDescription;
	unsigned int         numVertices;
	PLGVertex *          vertices;

	PLYFaceDescription  faceDescription;
	unsigned int        numFaces;
	PLYFaceDescription *faces;
} PLYHeader;

static unsigned int GetTypeForToken( const char *p )
{
	/* old vs new, though ive seen old more regularly used by blender etc. */
	if ( strcmp( p, "float" ) == 0 || strcmp( p, "float32" ) == 0 )
		return PLY_VAR_FLOAT;
	else if ( strcmp( p, "double" ) == 0 || strcmp( p, "float64" ) == 0 )
		return PLY_VAR_DOUBLE;
	else if ( strcmp( p, "int" ) == 0 || strcmp( p, "int32" ) == 0 )
		return PLY_VAR_INT;
	else if ( strcmp( p, "uint" ) == 0 || strcmp( p, "uint32" ) == 0 )
		return PLY_VAR_UINT;
	else if ( strcmp( p, "short" ) == 0 || strcmp( p, "int16" ) == 0 )
		return PLY_VAR_SHORT;
	else if ( strcmp( p, "ushort" ) == 0 || strcmp( p, "uint16" ) == 0 )
		return PLY_VAR_USHORT;
	else if ( strcmp( p, "char" ) == 0 || strcmp( p, "int8" ) == 0 )
		return PLY_VAR_CHAR;
	else if ( strcmp( p, "uchar" ) == 0 || strcmp( p, "uint8" ) == 0 )
		return PLY_VAR_UCHAR;

	return PLY_VAR_INVALID;
}

static PLMModel *ParseFile( const char *p )
{
	char token[ 64 ];
	if ( PlParseToken( &p, token, sizeof( token ) ) == NULL )
	{
		PrintWarn( "Failed to parse identifier!\n" );
		return NULL;
	}

	if ( strncmp( token, "ply", 3 ) != 0 )
	{
		PrintWarn( "Unexpected identifier, \"%s\"!\n", token );
		return NULL;
	}

	PLYHeader header;
	memset( &header, 0, sizeof( PLYHeader ) );

	/* parse the header */
	while ( PlParseToken( &p, token, sizeof( token ) ) != NULL )
	{
		if ( strcmp( token, "comment" ) == 0 )
		{
			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "format" ) == 0 )
		{
			/* ensure the format is what we support */
			PlParseToken( &p, token, sizeof( token ) );
			if ( strcmp( token, "ascii" ) != 0 )
			{
				PrintWarn( "Unexpected format, \"%s\"!\n", token );
				break;
			}

			PlParseToken( &p, token, sizeof( token ) );
			if ( strcmp( token, "1.0" ) != 0 )
			{
				PrintWarn( "Unexpected version, \"%s\"!\n", token );
				break;
			}

			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "element" ) == 0 )
		{
			PlParseToken( &p, token, sizeof( token ) );
			if ( strcmp( token, "vertex" ) == 0 )
			{
				header.numVertices = PlParseInteger( &p, NULL );
				if ( header.numVertices == 0 )
				{
					break;
				}

				header.vertices = globalSystem.CAlloc( header.numVertices, sizeof( PLGVertex ), true );

				PlSkipLine( &p );
				while ( PlParseToken( &p, token, sizeof( token ) ) != NULL )
				{
					if ( strcmp( token, "property" ) != 0 )
					{
						break;
					}

					PlParseToken( &p, token, sizeof( token ) );
					unsigned int type = GetTypeForToken( token );
					if ( type == PLY_VAR_INVALID )
					{
						PrintWarn( "Unexpected variable type, \"%s\"!\n", token );
						break;
					}
				}
			}
			else if ( strcmp( token, "face" ) == 0 )
			{
			}
			else
			{
				PrintWarn( "Unexpected element type, \"%s\"!\n", token );
			}

			PlSkipLine( &p );
			continue;
		}
	}

	if ( header.numFaces == 0 || header.faces == NULL )
	{
		globalSystem.Free( header.vertices );
		PrintWarn( "No faces outlined in ply header!\n" );
		return NULL;
	}
	else if ( header.numVertices == 0 || header.vertices == NULL )
	{
		globalSystem.Free( header.faces );
		PrintWarn( "No vertices outlined in ply header!\n" );
		return NULL;
	}
}

PLMModel *PLY_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		return NULL;
	}

	const char *p     = ( char * ) PlGetFileData( file );
	PLMModel *  model = ParseFile( p );

	PlCloseFile( file );

	return model;
}
