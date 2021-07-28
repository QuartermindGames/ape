/* ======================================================================
 * PkgMan, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * Purpose: Convert models from PLY, STL or other into .node:model format.
 * ====================================================================*/

#include <plcore/pl_parse.h>

#include "pkgman.h"

/* ======================================================================
 * STL Loader
 * ====================================================================*/

#define STL_TXT_HEADER_IDIF "solid "
#define STL_BIN_HEADER_SIZE 80

typedef struct STLTriangle
{
	PLVector3 normal;
	PLVector3 vertices[ 3 ];
	PLColour  colour;
	uint16_t  attribute;
} STLTriangle;

static PLMModel *MDL_STL_LoadASCIIFile( const char *buf, size_t length )
{
	/* todo */
	return NULL;
}

PLMModel *MDL_STL_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
		Error( "Failed to load STL: %s\nPL: %s\n", path, PlGetError() );

	char header[ STL_BIN_HEADER_SIZE ];
	if ( PlReadFile( file, header, sizeof( char ), STL_BIN_HEADER_SIZE ) != STL_BIN_HEADER_SIZE )
		Error( "Failed to read in header: %s\n", path );

	if ( strncmp( STL_TXT_HEADER_IDIF, header, strlen( STL_TXT_HEADER_IDIF ) ) == 0 )
	{
		size_t fileLength = PlGetFileSize( file );
		size_t length     = fileLength + 1;
		char * buf        = calloc( length, sizeof( char ) );

		PlFileSeek( file, 0, PL_SEEK_SET );
		if ( PlReadFile( file, buf, sizeof( char ), fileLength ) != fileLength )
			Error( "Failed to read in file: %s\nPL: %s\n", path, PlGetError() );

		PLMModel *model = MDL_STL_LoadASCIIFile( buf, length );

		free( buf );

		return model;
	}

	uint32_t     numTriangles = PlReadInt32( file, false, NULL );
	uint32_t     numVertices  = numTriangles * 3;
	STLTriangle *triangles    = calloc( numTriangles, sizeof( STLTriangle ) );
	if ( triangles == NULL )
		Error( "Failed to allocate triangles: %s " COM_FMT_uint32 "\n", path, numTriangles );

	for ( uint32_t i = 0; i < numTriangles; ++i )
	{
		bool status;
		for ( uint8_t j = 0; j < 3; ++j )
		{
			PlVectorIndex( triangles[ 0 ].normal, j ) = ( float ) PlReadInt32( file, false, &status );
			if ( !status )
				Error( "Failed to load in normal component: " COM_FMT_uint32 " " COM_FMT_uint16 "\n", i, j );
		}
		for ( uint8_t j = 0; j < 3; ++j )
		{
			for ( uint8_t k = 0; k < 3; ++k )
			{
				PlVectorIndex( triangles[ 0 ].vertices[ j ], k ) = ( float ) PlReadInt32( file, false, &status );
				if ( !status )
					Error( "Failed to load in vertex component: " COM_FMT_uint32 " " COM_FMT_uint16 "\n", i, j );
			}
		}

		triangles[ 0 ].attribute = PlReadInt16( file, false, &status );
		if ( !status )
			Error( "Failed to load in attribute component: " COM_FMT_uint32 "\n", i );

		triangles[ 0 ].colour.a = 255;
		if ( triangles[ 0 ].attribute != 0 )
		{
			/* todo: viscam/solidview encode the colours within the attribute var */
			triangles[ 0 ].colour.r = triangles[ 0 ].attribute;
			triangles[ 0 ].colour.g = triangles[ 0 ].attribute;
			triangles[ 0 ].colour.b = triangles[ 0 ].attribute;
		}
		else
		{
			triangles[ 0 ].colour.r = 255;
			triangles[ 0 ].colour.g = 255;
			triangles[ 0 ].colour.b = 255;
		}
	}

	/* and now convert that into our data */

	PLGMesh *mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, numVertices );
	if ( mesh == NULL )
		Error( "Failed to create mesh: %s\nPL: %s\n", path, PlGetError() );

	for ( uint32_t i = 0, nV = 0; i < numTriangles; ++i )
	{
		uint32_t indices[ 3 ];
		for ( uint8_t j = 0; j < 3; ++j )
			indices[ j ] = PlgAddMeshVertex( mesh, triangles[ i ].vertices[ j ], triangles[ i ].normal, triangles[ i ].colour, pl_vecOrigin2 );

		PlgAddMeshTriangle( mesh, indices[ 0 ], indices[ 1 ], indices[ 2 ] );
	}

	return PlmCreateBasicStaticModel( mesh );
}
