/* ======================================================================
 * PkgMan, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * Purpose: Convert models from PLY, STL or other into .node:model format.
 * ====================================================================*/

#include <plcore/pl_parse.h>

#include "pkgman.h"

/* ======================================================================
 * PLMModel > Node Conversion
 * ====================================================================*/

static PLMModel *MLD_ConvertNodeModelToPlatformModel( NLNode *root )
{
	/* todo */
}

NLNode *MDL_ConvertPlatformModelToNodeModel( const PLMModel *model )
{
	NLNode *root = NL_PushBackObj( NULL, "model" );

	NL_PushBackStrArray( root, "materials", ( const char ** ) model->materials, model->numMaterials );

	NLNode *meshArray = NL_PushBackObjArray( root, "meshes" );
	for ( uint8_t i = 0; i < model->numMeshes; ++i )
	{
		NLNode *mesh = NL_PushBackObj( meshArray, "mesh" );
		{
			NL_PushBackI32( mesh, "materialIndex", model->meshes[ i ]->materialIndex );

			NLNode *vertexArray   = NL_PushBackObjArray( mesh, "vertices" );
			for ( uint32_t j = 0; j < model->meshes[ i ]->num_verts; ++j )
            {
                NLNode *vertex = NL_PushBackObj( vertexArray, "vertex" );
                {
                    NLNode *vertexChild;
                    vertexChild = NL_PushBackObj( vertex, "position" );
                    {
                        NL_PushBackF32( vertexChild, "x", model->meshes[ i ]->vertices[ j ].position.x );
                        NL_PushBackF32( vertexChild, "y", model->meshes[ i ]->vertices[ j ].position.y );
                        NL_PushBackF32( vertexChild, "z", model->meshes[ i ]->vertices[ j ].position.z );
                    }
                    if ( !PlCompareVector3( &model->meshes[ i ]->vertices[ j ].normal, &pl_vecOrigin3 ) )
                    {
                        vertexChild = NL_PushBackObj( vertex, "normal" );
                        {
                            NL_PushBackF32( vertexChild, "x", model->meshes[ i ]->vertices[ j ].normal.x );
                            NL_PushBackF32( vertexChild, "y", model->meshes[ i ]->vertices[ j ].normal.y );
                            NL_PushBackF32( vertexChild, "z", model->meshes[ i ]->vertices[ j ].normal.z );
                        }
                    }
                    if ( !PlCompareColour( model->meshes[ i ]->vertices[ j ].colour, PLColour( 255, 255, 255, 255 ) ) )
                    {
                        vertexChild = NL_PushBackObj( vertex, "colour" );
                        {
                            NL_PushBackI8( vertexChild, "r", model->meshes[ i ]->vertices[ j ].colour.r );
                            NL_PushBackI8( vertexChild, "g", model->meshes[ i ]->vertices[ j ].colour.g );
                            NL_PushBackI8( vertexChild, "b", model->meshes[ i ]->vertices[ j ].colour.b );
                            NL_PushBackI8( vertexChild, "a", model->meshes[ i ]->vertices[ j ].colour.a );
                        }
                    }
                    if ( model->meshes[ i ]->vertices[ j ].bone_weight != 0.0f )
                    {
                        NL_PushBackF32( vertex, "boneWeight", model->meshes[ i ]->vertices[ j ].bone_weight );
                        NL_PushBackI32( vertex, "boneIndex", model->meshes[ i ]->vertices[ j ].bone_index );
                    }
                }
			}

			/* todo: this makes a rather crude assumption that all loaded meshes are
			 *  made up of triangles... */
			NLNode *triangleArray = NL_PushBackObjArray( mesh, "faces" );
			for ( uint32_t j = 0; j < model->meshes[ i ]->num_indices; j += 3 )
			{
				NLNode *face = NL_PushBackObj( triangleArray, "face" );
				NL_PushBackI32Array( face, "indices", ( int32_t * ) ( model->meshes[ i ]->indices + j ), 3 );
			}
		}
	}

	return root;
}

/* ======================================================================
 * GoldSrc MDL Loader
 * ====================================================================*/

#define MDL_NAME                 64
#define MDL_LABEL                32
#define MDL_MAX_BONE_CONTROLLERS 6

#define MDL_MAGIC     MAGIC_TO_NUM( 'I', 'D', 'S', 'T' )
#define MDL_SEQ_MAGIC MAGIC_TO_NUM( 'I', 'D', 'S', 'Q' )

#define MDL_VERSION 10

typedef struct VMDLHeader
{
	int32_t   magic; /* IDST / IDSQ */
	int32_t   version;
	char      name[ MDL_NAME ];
	int32_t   length;
	PLVector3 eyePos;
	PLVector3 min;
	PLVector3 max;
	PLVector3 bbMin;
	PLVector3 bbMax;
	int32_t   flags;
	int32_t   numBones;
	int32_t   boneIndex;
	int32_t   numBoneControllers;
	int32_t   boneControllerIndex;
	int32_t   numHitBoxes;
	int32_t   hitBoxIndex;
	int32_t   numAnims;
	int32_t   animIndex;
	int32_t   numAnimGroups;
	int32_t   animGroupIndex;
	int32_t   numTextures;
	int32_t   textureIndex;
	int32_t   textureDataIndex;
	int32_t   numSkinRefs;
	int32_t   numSkinFamilies;
	int32_t   skinIndex;
	int32_t   numBodyParts;
	int32_t   bodyPartIndex;
	int32_t   numAttachments;
	int32_t   attachmentIndex;
	int32_t   soundTable;
	int32_t   soundIndex;
	int32_t   numSoundGroups;
	int32_t   soundGroupIndex;
	int32_t   numTransitions;
	int32_t   transitionIndex;
} VMDLHeader;

typedef struct VMDLBoundingBox
{
	int32_t   bone;
	int32_t   group;
	PLVector3 bbMin;
	PLVector3 bbMax;
} VMDLBoundingBox;

typedef struct VMDLAnimationHeader
{
	int32_t id;
	int32_t version;
	char    name[ MDL_NAME ];
	int32_t length;
} VMDLAnimationHeader;

typedef struct VMDLAnimationGroup
{
	char    label[ MDL_LABEL ];
	char    name[ MDL_NAME ];
	int32_t unused[ 2 ];
} VMDLAnimationGroup;

typedef struct VMDLBone
{
	char    label[ MDL_LABEL ];
	int32_t parent;
	int32_t flags;
	int32_t boneControllers[ MDL_MAX_BONE_CONTROLLERS ];
	float   values[ MDL_MAX_BONE_CONTROLLERS ];
	float   scale[ MDL_MAX_BONE_CONTROLLERS ];
} VMDLBone;

typedef struct VMDLBoneController
{
	int32_t bone;
	int32_t type;
	float   range[ 2 ];
	int32_t rest;
	int32_t index;
} VMDLBoneController;

PLMModel *MDL_MDL_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
		return NULL;

	VMDLHeader header;
	if ( PlReadFile( file, &header, sizeof( VMDLHeader ), 1 ) != 1 )
		Error( "Failed to read in header: %s\nPL: %s\n", path, PlGetError() );

	/* now carry out some basic validation */

	if ( header.magic != MDL_MAGIC && header.magic != MDL_SEQ_MAGIC )
		Error( "Invalid identifier for MDL: %d vs %d!\n", header.magic, MDL_MAGIC );
}

/* ======================================================================
 * MD2 Loader
 * ====================================================================*/

#define MD2_MAGIC   MAGIC_TO_NUM( 'I', 'D', 'P', '2' )
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
        const char *       skinPath,
        const MD2Header *  header,
        const MD2TexCoord *texCoords,
        const MD2Triangle *triangles,
        const MD2Frame *   frames )
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
		Error( "Failed to load MD2: %s\nPL: %s\n", path, PlGetError() );

	/* there shouldn't be any padding here, so just read
	 * the whole header in */
	MD2Header header;
	memset( &header, 0, sizeof( MD2Header ) );
	if ( PlReadFile( file, &header, sizeof( header ), 1 ) != 1 )
		Error( "Failed to read in header: %s\nPL: %s\n", path, PlGetError() );

	if ( header.magic != MD2_MAGIC )
		Error( "Invalid identifier for MD2: %d vs %d!\n", header.magic, MD2_MAGIC );
	if ( header.version != MD2_VERSION )
		Error( "Invalid version for MD2: %d vs %d!\n", header.version, MD2_VERSION );

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
		Error( "Failed to allocate frames: %s\n", path );
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
		Error( "Failed to create mesh: %s\nPL: %s\n", path, PlGetError() );

	/* free the original data */

	free( texCoords );
	free( triangles );
	for ( int32_t i = 0; i < header.numFrames; ++i ) free( frames[ i ].vertices );
	free( frames );

	PLMModel *model = PlmCreateBasicStaticModel( mesh );
	PlmGenerateModelBounds( model );

	return model;
}

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
        Error( "Failed to parse identifier!\n" );

    if ( strncmp( token, "ply", 3 ) != 0 )
        Error( "Unexpected identifier, \"%s\"!\n", token );

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
                Error( "Unexpected format, \"%s\"!\n", token );

            PlParseToken( &p, token, sizeof( token ) );
            if ( strcmp( token, "1.0" ) != 0 )
                Error( "Unexpected version, \"%s\"!\n", token );

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
                    break;

                header.vertices = calloc( header.numVertices, sizeof( PLGVertex ) );

                PlSkipLine( &p );
                while ( PlParseToken( &p, token, sizeof( token ) ) != NULL )
                {
                    if ( strcmp( token, "property" ) != 0 )
                        break;

                    PlParseToken( &p, token, sizeof( token ) );
                    unsigned int type = GetTypeForToken( token );
                    if ( type == PLY_VAR_INVALID )
                        Error( "Unexpected variable type, \"%s\"!\n", token );
                }
            }
            else if ( strcmp( token, "face" ) == 0 )
            {
            }
            else
                Error( "Unexpected element type, \"%s\"!\n", token );

            PlSkipLine( &p );
            continue;
        }
    }

    if ( header.numFaces == 0 || header.faces == NULL )
        Error( "No faces outlined in ply header!\n" );

    if ( header.numVertices == 0 || header.vertices == NULL )
        Error( "No vertices outlined in ply header!\n" );
}

PLMModel *MDL_PLY_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
		Error( "Failed to load PLY: %s\nPL: %s\n", path, PlGetError() );

	PLMModel *model = ParseFile( ( char * ) PlGetFileData( file ) );
	if ( model == NULL )
		Error( "Failed to parse PLY: %s\n", path );

	PlCloseFile( file );

	return model;
}
