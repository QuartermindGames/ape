/* ======================================================================
 * PkgMan, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * Purpose: Convert models from PLY, STL or other into .node:model format.
 * ====================================================================*/

#include <plcore/pl_parse.h>

#include "pkgman.h"

/* ======================================================================
 * PLY Loader
 * ====================================================================*/

typedef enum PLYVariableType
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
} PLYVariableType;

typedef struct PLYVariable
{
	PLYVariableType type;
	union
	{
		float		   f;
		double		   f64;
		double		   d;
		unsigned char  uc;
		char		   c;
		unsigned short us;
		short		   s;
		unsigned int   ui;
		int			   i;
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
	unsigned int		 numVertices;
	PLGVertex			  *vertices;

	PLYFaceDescription	faceDescription;
	unsigned int		numFaces;
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
		Error( "Failed to parse identifier!\n" );

	if ( strncmp( token, "ply", 3 ) != 0 )
		Error( "Unexpected identifier, \"%s\"!\n", token );

	PLYHeader header;
	memset( &header, 0, sizeof( PLYHeader ) );

	/* parse the header */
	while ( PlParseToken( &p, token, sizeof( token ) ) != NULL )
	{
		if ( *p == '\0' )
		{
			break;
		}

		if ( strcmp( token, "comment" ) == 0 || *token == '\0' )
		{
		}
		else if ( strcmp( token, "format" ) == 0 )
		{
			/* ensure the format is what we support */
			PlParseToken( &p, token, sizeof( token ) );
			if ( strcmp( token, "ascii" ) != 0 && strcmp( token, "binary_little_endian" ) != 0 )
				Error( "Unexpected format, \"%s\"!\n", token );

			PlParseToken( &p, token, sizeof( token ) );
			if ( strcmp( token, "1.0" ) != 0 )
				Error( "Unexpected version, \"%s\"!\n", token );
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

				header.vertices = calloc( header.numVertices, sizeof( PLGVertex ) );

				PlSkipLine( &p );
				while ( PlParseToken( &p, token, sizeof( token ) ) != NULL )
				{
					if ( strcmp( token, "property" ) != 0 )
					{
						break;
					}

					/* type */
					PlParseToken( &p, token, sizeof( token ) );
					unsigned int type = GetTypeForToken( token );
					if ( type == PLY_VAR_INVALID )
					{
						Error( "Unexpected variable type, \"%s\"!\n", token );
					}

					PlSkipLine( &p );

					/* name */
					PlParseToken( &p, token, sizeof( token ) );
					if ( strcmp( token, "x" ) == 0 ) header.vertexDescription.x.type = type;
					if ( strcmp( token, "y" ) == 0 ) header.vertexDescription.y.type = type;
					if ( strcmp( token, "z" ) == 0 ) header.vertexDescription.z.type = type;
					if ( strcmp( token, "nx" ) == 0 ) header.vertexDescription.nx.type = type;
					if ( strcmp( token, "ny" ) == 0 ) header.vertexDescription.ny.type = type;
					if ( strcmp( token, "nz" ) == 0 ) header.vertexDescription.nz.type = type;
					if ( strcmp( token, "s" ) == 0 ) header.vertexDescription.s.type = type;
					if ( strcmp( token, "t" ) == 0 ) header.vertexDescription.t.type = type;
					Print( "PLY vertex property, \"%s\", of type %d\n", token, type );

					PlSkipLine( &p );
				}

				continue;
			}
			else if ( strcmp( token, "face" ) == 0 )
			{
				header.numFaces = PlParseInteger( &p, NULL );
				if ( header.numFaces == 0 )
					break;

				header.faces = calloc( header.numFaces, sizeof( PLYFaceDescription ) );

				PlSkipLine( &p );
				while ( PlParseToken( &p, token, sizeof( token ) ) != NULL )
				{
				}
			}
			else
				Error( "Unexpected element type, \"%s\"!\n", token );

			PlSkipLine( &p );
		}

		PlSkipLine( &p );
	}

	if ( header.numFaces == 0 || header.faces == NULL )
		Error( "No faces outlined in ply header!\n" );

	if ( header.numVertices == 0 || header.vertices == NULL )
		Error( "No vertices outlined in ply header!\n" );
}

PLMModel *MDL_PLY_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
		Error( "Failed to load PLY: %s\nPL: %s\n", path, PlGetError() );

	const char *p	  = ( char	   *) PlGetFileData( file );
	PLMModel	 *model = ParseFile( p );
	if ( model == NULL )
		Error( "Failed to parse PLY: %s\n", path );

	PlCloseFile( file );

	return model;
}
