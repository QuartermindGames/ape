/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/platform_model.h>

#include "yin.h"

#define MAGIC_TO_NUM( A, B, C, D ) ( ( D << 24 ) + ( C << 16 ) + ( B << 8 ) + A )

#define MD2_MAGIC MAGIC_TO_NUM( 'I', 'D', 'P', '2' )
#define MD2_VERSION 8

typedef struct MD2Header {
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

typedef struct MD2TexCoord {
	int16_t s, t;
} MD2TexCoord;

typedef struct MD2Triangle {
	uint16_t vertex[ 3 ];
	uint16_t st[ 3 ];
} MD2Triangle;

typedef struct MD2Vertex {
	uint8_t v[ 3 ];
	uint8_t normalIndex;
} MD2Vertex;

typedef struct MD2Frame {
	PLVector3 scale;
	PLVector3 translate;
	char name[ 16 ];
	MD2Vertex *vertices;
} MD2Frame;

static PLMesh *MD2_ConvertMD2ToMesh(
        const char *skinPath,
        const MD2Header *header,
        const MD2TexCoord *texCoords,
        const MD2Triangle *triangles,
        const MD2Frame *frames ) {
	PLMesh *mesh = plCreateMesh( PL_MESH_TRIANGLES, PL_DRAW_STATIC, header->numTriangles, header->numVertices );
	if ( mesh == NULL ) {
		return NULL;
	}

	/* fetch the width and height, we'll need these to convert
	 * uv coords */
	int w = 256, h = 256;
	PLImage *image = plLoadImage( skinPath );
	if ( image != NULL ) {
		w = image->width;
		h = image->height;
		plDestroyImage( image );
	}

	/* setup the vertex table */
	for ( unsigned int i = 0; i < header->numVertices; ++i ) {
		PLVector3 pos;
		pos.x = ( frames[ 0 ].scale.x * frames[ 0 ].vertices[ i ].v[ 0 ] ) + frames[ 0 ].translate.x;
        pos.y = ( frames[ 0 ].scale.y * frames[ 0 ].vertices[ i ].v[ 1 ] ) + frames[ 0 ].translate.y;
        pos.z = ( frames[ 0 ].scale.z * frames[ 0 ].vertices[ i ].v[ 2 ] ) + frames[ 0 ].translate.z;

		plAddMeshVertex( mesh, pos, pl_vecOrigin3, PL_COLOUR_WHITE, pl_vecOrigin2 );
        printf( "%s\n", plPrintVector3( &pos, pl_float_var ) );
	}

    for ( unsigned int i = 0; i < header->numTriangles; ++i ) {
		const MD2Triangle *tri = &triangles[ i ];
		/* setup the uv coords */
        for ( unsigned int j = 0; j < 3; ++j ) {
			PLVertex *v = &mesh->vertices[ tri->vertex[ 0 ] ];
			v->st[ 0 ].x = texCoords[ tri->st[ j ] ].s / w;
			v->st[ 0 ].y = texCoords[ tri->st[ j ] ].t / h;
		}
		plAddMeshTriangle( mesh, tri->vertex[ 0 ], tri->vertex[ 1 ], tri->vertex[ 2 ] );
		printf( "%d %d %d\n", tri->vertex[ 0 ], tri->vertex[ 1], tri->vertex[ 2 ] );
	}

	/* MD2 models don't really have normals, they instead use a pre-computed table
	 * so we'll generate them manually instead */

	plGenerateMeshNormals( mesh, false );
	plGenerateMeshTangentBasis( mesh );

	return mesh;
}

/**
 * Loads in an MD2 mesh, currently just as a static thing.
 * This implementation currently doesn't account for endianness! :(
 */
PLModel *MD2_LoadFile( const char *path ) {
	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

	/* there shouldn't be any padding here, so just read
	 * the whole header in */
	MD2Header header;
	memset( &header, 0, sizeof( MD2Header ) );
	plReadFile( file, &header, sizeof( header ), 1 );
	if ( header.magic != MD2_MAGIC ) {
		PrintWarn( "Invalid identifier for MD2: %d vs %d!\n", header.magic, MD2_MAGIC );
		return NULL;
	} else if ( header.version != MD2_VERSION ) {
		PrintWarn( "Invalid version for MD2: %d vs %d!\n", header.version, MD2_VERSION );
		return NULL;
	}

	/* read in all the skins
	 * this only loads in the first skin, as MD2s only actually use 1 skin
	 * per mesh - we'll need to load this later to convert the uv coords */
	MD2Skin skin;
	plFileSeek( file, header.offsetSkins, PL_SEEK_SET );
	plReadFile( file, skin, sizeof( MD2Skin ), 1 );

	/* and now read in all the tex coordinates */
	MD2TexCoord *texCoords = ( MD2TexCoord * ) AllocMemory( sizeof( MD2TexCoord ) * header.numST, true );
	plFileSeek( file, header.offsetST, PL_SEEK_SET );
	plReadFile( file, texCoords, sizeof( MD2TexCoord ), header.numST );

	/* triangles */
	MD2Triangle *triangles = ( MD2Triangle * ) AllocMemory( sizeof( MD2Triangle ) * header.numTriangles, true );
	plFileSeek( file, header.offsetTriangles, PL_SEEK_SET );
	plReadFile( file, triangles, sizeof( MD2Triangle ), header.numTriangles );

	/* frames */
	MD2Frame *frames = ( MD2Frame * ) AllocMemory( sizeof( MD2Frame ) * header.numFrames, true );
	plFileSeek( file, header.offsetFrames, PL_SEEK_SET );
	for ( unsigned int i = 0; i < header.numFrames; ++i ) {
		plReadFile( file, &frames[ i ].scale, sizeof( PLVector3 ), 1 );
		plReadFile( file, &frames[ i ].translate, sizeof( PLVector3 ), 1 );
		plReadFile( file, &frames[ i ].name, sizeof( char ), sizeof( frames[ i ].name ) );

        frames[ i ].vertices = ( MD2Vertex * ) AllocMemory( sizeof( MD2Vertex ) * header.numVertices, true );
		plReadFile( file, frames[ i ].vertices, sizeof( MD2Vertex ), header.numVertices );
	}

	plCloseFile( file );

	/* map the skin name to our materials/models/ directory */
	char fileName[ 32 ];
	plStripExtension( fileName, sizeof( fileName ), plGetFileName( path ) );
    char fullSkinPath[ PL_SYSTEM_MAX_PATH ];
    snprintf( fullSkinPath, sizeof( fullSkinPath ), "materials/models/%s/%s", fileName, skin );
	pl_strtolower( fullSkinPath );

	/* and now we need to convert all this into a PLModel */
	PLMesh *mesh = MD2_ConvertMD2ToMesh( fullSkinPath, &header, texCoords, triangles, frames );

	/* free the original data */

	free( texCoords );
	free( triangles );
	for ( unsigned int i = 0; i < header.numFrames; ++i ) free( frames[ i ].vertices );
	free( frames );

	if ( mesh == NULL ) {
		PrintWarn( "Failed to generate mesh structure from MD2! Check log for details.\n" );
		return NULL;
	}

	PLModel *model = plCreateBasicStaticModel( mesh );
	plGenerateModelBounds( model );

	return model;
}
