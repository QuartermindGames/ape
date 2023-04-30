// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
/* ======================================================================
 * MD2 Loader
 * ====================================================================*/

#include "modelconv.h"

#define MD2_MAGIC   PL_MAGIC_TO_NUM( 'I', 'D', 'P', '2' )
#define MD2_VERSION 8

typedef struct MD2Header
{
	int32_t magic;
	int32_t version;
	int32_t skinWidth;
	int32_t skinHeight;
	int32_t frameSize;
	int32_t numSkins;
	int32_t numVertices;
	int32_t numST;
	int32_t numTriangles;
	int32_t numGlCmds;
	int32_t numFrames;
	int32_t offsetSkins;
	int32_t offsetST;
	int32_t offsetTriangles;
	int32_t offsetFrames;
	int32_t offsetGlCmds;
	int32_t offsetEnd;
} MD2Header;

typedef char MD2Skin[ 64 ];

typedef struct MD2TexCoord
{
	int16_t s, t;
} MD2TexCoord;

typedef struct MD2Triangle
{
	uint16_t vertex[ 3 ];
	uint16_t st[ 3 ];
} MD2Triangle;

typedef struct MD2Vertex
{
	uint8_t v[ 3 ];
	uint8_t normalIndex;
} MD2Vertex;

typedef struct MD2Frame
{
	PLVector3  scale;
	PLVector3  translate;
	char       name[ 16 ];
	MD2Vertex *vertices;
} MD2Frame;

static PLGMesh *MDL_MD2_ConvertMD2ToMesh(
		const char        *skinPath,
		const MD2Header   *header,
		const MD2TexCoord *texCoords,
		const MD2Triangle *triangles,
		const MD2Frame    *frames )
{
	PLGMesh *mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, header->numTriangles, header->numVertices );
	if ( mesh == NULL )
		return NULL;

	/* fetch the width and height, we'll need these to convert
     * uv coords */
	int      w = 256, h = 256;
	PLImage *image = PlLoadImage( skinPath );
	if ( image != NULL )
	{
		w = image->width;
		h = image->height;
		PlDestroyImage( image );
	}

	/* setup the vertex table */
	for ( int32_t i = 0; i < header->numVertices; ++i )
	{
		PLVector3 pos;
		pos.x = ( frames[ 0 ].scale.x * frames[ 0 ].vertices[ i ].v[ 0 ] ) + frames[ 0 ].translate.x;
		pos.y = ( frames[ 0 ].scale.y * frames[ 0 ].vertices[ i ].v[ 1 ] ) + frames[ 0 ].translate.y;
		pos.z = ( frames[ 0 ].scale.z * frames[ 0 ].vertices[ i ].v[ 2 ] ) + frames[ 0 ].translate.z;

		PlgAddMeshVertex( mesh, pos, pl_vecOrigin3, PL_COLOUR_WHITE, pl_vecOrigin2 );
		//Print( "%s\n", PlPrintVector3( &pos, pl_float_var ) );
	}

	for ( int32_t i = 0; i < header->numTriangles; ++i )
	{
		const MD2Triangle *tri = &triangles[ i ];
		/* setup the uv coords */
		for ( uint8_t j = 0; j < 3; ++j )
		{
			PLGVertex *v = &mesh->vertices[ tri->vertex[ 0 ] ];
			v->st[ 0 ].x = ( float ) ( texCoords[ tri->st[ j ] ].s / w );
			v->st[ 0 ].y = ( float ) ( texCoords[ tri->st[ j ] ].t / h );
		}
		PlgAddMeshTriangle( mesh, tri->vertex[ 0 ], tri->vertex[ 1 ], tri->vertex[ 2 ] );
		//Print( "%d %d %d\n", tri->vertex[ 0 ], tri->vertex[ 1 ], tri->vertex[ 2 ] );
	}

	/* MD2 models don't really have normals, they instead use a pre-computed table
     * so we'll generate them manually instead */

	PlgGenerateMeshNormals( mesh, false );
	PlgGenerateMeshTangentBasis( mesh );

	return mesh;
}

PLMModel *MDL_MD2_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		Error( "Failed to load MD2: %s\nPL: %s\n", path, PlGetError() );
	}

	/* there shouldn't be any padding here, so just read
     * the whole header in */
	MD2Header header;
	memset( &header, 0, sizeof( MD2Header ) );
	if ( PlReadFile( file, &header, sizeof( header ), 1 ) != 1 )
	{
		Error( "Failed to read in header: %s\nPL: %s\n", path, PlGetError() );
	}

	if ( header.magic != MD2_MAGIC )
	{
		Error( "Invalid identifier for MD2: %d vs %d!\n", header.magic, MD2_MAGIC );
	}
	if ( header.version != MD2_VERSION )
	{
		Error( "Invalid version for MD2: %d vs %d!\n", header.version, MD2_VERSION );
	}

	/* read in all the skins
     * this only loads in the first skin, as MD2s only actually use 1 skin
     * per mesh - we'll need to load this later to convert the uv coords */
	MD2Skin skin;
	PlFileSeek( file, header.offsetSkins, PL_SEEK_SET );
	PlReadFile( file, skin, sizeof( MD2Skin ), 1 );

	/* and now read in all the tex coordinates */
	MD2TexCoord *texCoords = ( MD2TexCoord * ) malloc( sizeof( MD2TexCoord ) * header.numST );
	PlFileSeek( file, header.offsetST, PL_SEEK_SET );
	PlReadFile( file, texCoords, sizeof( MD2TexCoord ), header.numST );

	/* triangles */
	MD2Triangle *triangles = ( MD2Triangle * ) malloc( sizeof( MD2Triangle ) * header.numTriangles );
	PlFileSeek( file, header.offsetTriangles, PL_SEEK_SET );
	PlReadFile( file, triangles, sizeof( MD2Triangle ), header.numTriangles );

	/* frames */
	MD2Frame *frames = ( MD2Frame * ) malloc( sizeof( MD2Frame ) * header.numFrames );
	if ( frames == NULL )
	{
		Error( "Failed to allocate frames: %s\n", path );
	}
	PlFileSeek( file, header.offsetFrames, PL_SEEK_SET );
	for ( int32_t i = 0; i < header.numFrames; ++i )
	{
		PlReadFile( file, &frames[ i ].scale, sizeof( PLVector3 ), 1 );
		PlReadFile( file, &frames[ i ].translate, sizeof( PLVector3 ), 1 );
		PlReadFile( file, &frames[ i ].name, sizeof( char ), sizeof( frames[ i ].name ) );

		frames[ i ].vertices = ( MD2Vertex * ) malloc( sizeof( MD2Vertex ) * header.numVertices );
		PlReadFile( file, frames[ i ].vertices, sizeof( MD2Vertex ), header.numVertices );
	}

	PlCloseFile( file );

	/* map the skin name to our materials/models/ directory */
	char fileName[ 32 ];
	PlStripExtension( fileName, sizeof( fileName ), PlGetFileName( path ) );
	char fullSkinPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( fullSkinPath, sizeof( fullSkinPath ), "materials/models/%s/%s", fileName, skin );
	pl_strtolower( fullSkinPath );

	/* and now we need to convert all this into a PLModel */
	PLGMesh *mesh = MDL_MD2_ConvertMD2ToMesh( fullSkinPath, &header, texCoords, triangles, frames );
	if ( mesh == NULL )
	{
		Error( "Failed to create mesh: %s\nPL: %s\n", path, PlGetError() );
	}

	/* free the original data */

	free( texCoords );
	free( triangles );
	for ( int32_t i = 0; i < header.numFrames; ++i ) free( frames[ i ].vertices );
	free( frames );

	PLMModel *model = PlmCreateBasicStaticModel( mesh );
	PlmGenerateModelBounds( model );

	return model;
}
