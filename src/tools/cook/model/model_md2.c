// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: MD2 loader - carried over from some older code I wrote.
// Author:  Mark E. Sowden

#include <plcore/pl_filesystem.h>

#include "../cook.h"
#include "model.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

#define MD2_MAGIC   QM_OS_MAGIC_TO_NUM( 'I', 'D', 'P', '2' )
#define MD2_VERSION 8

typedef struct Md2Header
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
} Md2Header;

typedef char Md2Skin[ 64 ];

typedef struct Md2TexCoord
{
	int16_t s, t;
} Md2TexCoord;

typedef struct Md2Triangle
{
	uint16_t vertex[ 3 ];
	uint16_t st[ 3 ];
} Md2Triangle;

typedef struct Md2Vertex
{
	uint8_t v[ 3 ];
	uint8_t normalIndex;
} Md2Vertex;

typedef struct Md2Frame
{
	QmMathVector3f  scale;
	QmMathVector3f  translate;
	char       name[ 16 ];
	Md2Vertex *vertices;
} Md2Frame;

typedef struct Md2Model
{
	Md2Skin skin;

	Md2TexCoord *texCoords;
	unsigned int numTexCoords;

	Md2Triangle *triangles;
	unsigned int numTriangles;

	Md2Frame    *frames;
	unsigned int numFrames;
} Md2Model;

static Md2Model *parse_md2( PLFile *file )
{
	Md2Header header = {};
	if ( PlReadFile( file, &header, sizeof( header ), 1 ) != 1 )
	{
		WARN( "Failed to read in MD2 header: %s\n", PlGetError() );
		return nullptr;
	}

	if ( header.magic != MD2_MAGIC )
	{
		WARN( "Invalid magic for MD2 (%u != %u)!\n", header.magic, MD2_MAGIC );
		return nullptr;
	}
	if ( header.version != MD2_VERSION )
	{
		WARN( "Invalid version for MD2 (%u != %u)!\n", header.version, MD2_VERSION );
		return nullptr;
	}

	/* read in all the skins
     * this only loads in the first skin, as MD2s only actually use 1 skin
     * per mesh - we'll need to load this later to convert the uv coords */
	Md2Skin skin;
	PlFileSeek( file, header.offsetSkins, PL_SEEK_SET );
	if ( PlReadFile( file, skin, sizeof( Md2Skin ), 1 ) != 1 )
	{
		WARN( "Failed to read in MD2 skin: %s\n", PlGetError() );
		return nullptr;
	}

	/* and now read in all the tex coordinates */
	Md2TexCoord *texCoords = QM_OS_MEMORY_NEW_( Md2TexCoord, header.numST );
	PlFileSeek( file, header.offsetST, PL_SEEK_SET );
	PlReadFile( file, texCoords, sizeof( Md2TexCoord ), header.numST );

	/* triangles */
	Md2Triangle *triangles = QM_OS_MEMORY_NEW_( Md2Triangle, header.numTriangles );
	PlFileSeek( file, header.offsetTriangles, PL_SEEK_SET );
	PlReadFile( file, triangles, sizeof( Md2Triangle ), header.numTriangles );

	/* frames */
	Md2Frame *frames = QM_OS_MEMORY_NEW_( Md2Frame, header.numFrames );
	PlFileSeek( file, header.offsetFrames, PL_SEEK_SET );
	for ( int32_t i = 0; i < header.numFrames; ++i )
	{
		PlReadFile( file, &frames[ i ].scale, sizeof( QmMathVector3f ), 1 );
		PlReadFile( file, &frames[ i ].translate, sizeof( QmMathVector3f ), 1 );
		PlReadFile( file, &frames[ i ].name, sizeof( char ), sizeof( frames[ i ].name ) );

		frames[ i ].vertices = QM_OS_MEMORY_NEW_( Md2Vertex, header.numVertices );
		PlReadFile( file, frames[ i ].vertices, sizeof( Md2Vertex ), header.numVertices );
	}

	Md2Model *model = QM_OS_MEMORY_NEW( Md2Model );

	model->numFrames = header.numFrames;
	model->frames    = frames;

	model->numTriangles = header.numTriangles;
	model->triangles    = triangles;

	model->numTexCoords = header.numST;
	model->texCoords    = texCoords;

	return model;
}

static Md2Model *model_md2_load( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == nullptr )
	{
		WARN( "Failed to load MD2 \"%s\": %s\n", path, PlGetError() );
		return nullptr;
	}

	Md2Model *model = parse_md2( file );
	PlCloseFile( file );
	return model;
}

static void model_md2_destroy( Md2Model *self )
{
	for ( unsigned int i = 0; i < self->numFrames; ++i )
	{
		qm_os_memory_free( self->frames[ i ].vertices );
	}

	qm_os_memory_free( self->frames );
	qm_os_memory_free( self->triangles );
	qm_os_memory_free( self->texCoords );
	qm_os_memory_free( self );
}

static CookModel *md2_to_ape( const Md2Model *model, CookModel *out )
{
	//todo
#if 0
	/* fetch the width and height, we'll need these to convert
     * uv coords */
	int w = 256, h = 256;
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
		QmMathVector3f pos;
		pos.x = ( frames[ 0 ].scale.x * frames[ 0 ].vertices[ i ].v[ 0 ] ) + frames[ 0 ].translate.x;
		pos.y = ( frames[ 0 ].scale.y * frames[ 0 ].vertices[ i ].v[ 1 ] ) + frames[ 0 ].translate.y;
		pos.z = ( frames[ 0 ].scale.z * frames[ 0 ].vertices[ i ].v[ 2 ] ) + frames[ 0 ].translate.z;

		PlgAddMeshVertex( mesh, &pos, &QM_MATH_VECTOR3F_ZERO, &PL_COLOUR_WHITE, &QM_MATH_VECTOR2F_ZERO );
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
#endif

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////

static CookModel *load_md2( const char *path )
{
	return ( CookModel * ) model_md2_load( path );
}

static void destroy_md2( CookModel *self )
{
	model_md2_destroy( ( Md2Model * ) self );
}

static CookModel *conv_md2( const CookModel *model, CookModel *out )
{
	return md2_to_ape( ( const Md2Model * ) model, out );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

const CookModelFormatInterface modelMd2Interface = {
        "md2",
        load_md2,
        conv_md2,
        destroy_md2,
};
