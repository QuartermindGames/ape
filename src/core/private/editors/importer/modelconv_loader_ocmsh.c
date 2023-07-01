// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "modelconv.h"

#define OC_MAGIC PL_MAGIC_TO_NUM( 'q', 'n', '\0', '\0' )

typedef struct OCVertex
{
	float x;
	float y;
	float z;
	float w; /* perhaps weight? */
} OCVertex;

#define MAX_VERTICES UINT16_MAX
static OCVertex ocVertices[ MAX_VERTICES ];

static void OC_MSH_ParseVertices( PLFile *file, int32_t *verticesNum )
{
	int32_t magic = PlReadInt32( file, false, NULL );
	if ( magic != OC_MAGIC )
	{
		Error( "Unexpected magic: %d vs %d\n", magic, OC_MAGIC );
	}

	if ( !PlFileSeek( file, 24, PL_SEEK_CUR ) )
	{
		Error( "Failed to seek to vertices!\n" );
	}

	int32_t numVertices = PlReadInt32( file, false, NULL );
	if ( numVertices <= 0 || numVertices >= MAX_VERTICES )
	{
		Error( "Invalid number of vertices in msh!\n" );
	}

	for ( int32_t i = 0; i < numVertices; ++i )
	{
		ocVertices[ i ].x = PlReadFloat32( file, false, NULL );
		ocVertices[ i ].y = PlReadFloat32( file, false, NULL );
		ocVertices[ i ].z = PlReadFloat32( file, false, NULL );
		ocVertices[ i ].w = PlReadFloat32( file, false, NULL );
	}

	*verticesNum = numVertices;
}

typedef struct OCFace
{
	int16_t x;
	int16_t y;
	int16_t z;
	int16_t textureId;
} OCFace;

#define MAX_FACES UINT16_MAX
static OCFace ocFaces[ MAX_FACES ];

static void OC_MSH_ParseFaces( PLFile *file, int32_t *facesNum )
{
	int32_t numFaces = PlReadInt32( file, false, NULL );
	if ( numFaces <= 0 || numFaces >= MAX_FACES )
	{
		Error( "Invalid number of faces in msh!\n" );
	}

	for ( int32_t i = 0; i < numFaces; ++i )
	{
		ocFaces[ i ].x = PlReadInt16( file, false, NULL );
		ocFaces[ i ].y = PlReadInt16( file, false, NULL );
		ocFaces[ i ].z = PlReadInt16( file, false, NULL );
		ocFaces[ i ].textureId = PlReadInt16( file, false, NULL );
	}

	*facesNum = numFaces;
}

static PLMModel *OC_MSH_ParseFile( PLFile *file )
{
	int32_t magic = PlReadInt32( file, false, NULL );
	if ( magic != OC_MAGIC )
	{
		printf( "Unexpected magic: %d vs %d\n", magic, OC_MAGIC );
		return NULL;
	}

	/* these seem to provide some sort of metadata for
	 * identifying the file type? noticed other files
	 * feature the same magic, but then these same bytes
	 * are different depending on the type. interesting. */
	if ( !PlFileSeek( file, 20, PL_SEEK_CUR ) )
	{
		printf( "Failed to seek to table!\n" );
		return NULL;
	}

	int32_t fileSize = PlReadInt32( file, false, NULL );
	int32_t realFileSize = ( int32_t ) PlGetFileSize( file );
	if ( fileSize != realFileSize )
	{
		printf( "Unexpected file size indicated in header: %d vs %d\n", fileSize, realFileSize );
		return NULL;
	}

	int32_t numVertices;
	OC_MSH_ParseVertices( file, &numVertices );
	int32_t numTriangles;
	OC_MSH_ParseFaces( file, &numTriangles );

	PLGMesh **meshes = PlCAllocA( 1, sizeof( PLGMesh * ) );
	PLMModel *model = PlmCreateStaticModel( meshes, 1 );
	if ( model == NULL )
	{
		Error( "Failed to create model container!\nPL: %s\n", PlGetError() );
	}

	meshes[ 0 ] = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, numTriangles, numVertices );
	meshes[ 0 ]->materialIndex = 0;

	/* we've swapped the y and z below on purpose to match Yin's awkward coordinates ._. */

	for ( int32_t i = 0; i < numVertices; ++i )
	{
		PlgAddMeshVertex( meshes[ 0 ],
		                  PLVector3(
		                          ocVertices[ i ].x,
		                          ocVertices[ i ].z,
		                          ocVertices[ i ].y ),
		                  pl_vecOrigin3,
		                  PLColourRGB( 255, 255, 255 ),
		                  PLVector2( PlByteToFloat( rand() % 255 ), PlByteToFloat( rand() % 255 ) ) );
	}

	for ( int32_t i = 0; i < numTriangles; ++i )
	{
		PlgAddMeshTriangle( meshes[ 0 ], ocFaces[ i ].x, ocFaces[ i ].z, ocFaces[ i ].y );
	}

	model->numMaterials = 1;
	model->materials = PlMAllocA( sizeof( PLPath ) * model->numMaterials );
	snprintf( model->materials[ 0 ], sizeof( PLPath ), "materials/editor/default.mat.n" );

	PlmGenerateModelNormals( model, true );

	return model;
}

PLMModel *OC_MSH_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		Error( "Failed to load MSH \"%s\"!\nPL: %s\n", path, PlGetError() );
	}

	PLMModel *model = OC_MSH_ParseFile( file );
	if ( model == NULL )
	{
		Error( "Failed to parse MSH \"%s\"!\n", PlGetError() );
	}

	PlCloseFile( file );

	return model;
}
