// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Loader for Hogs of War's PC model format.
//			Based on my previous work here:
//			https://github.com/TalonBraveInfo/OpenHoW/blob/master/src/engine/loaders/
// Author:  Mark E. Sowden

#include "../cook.h"

#include "model.h"

/* Hogs of War file formats
 * -----------------------------------
 * BIN : Audio/Model/Texture data
 * DAT : Audio data
 * MAM : Mangled texture / model package
 * MAD : Texture / model package            (done)
 * MTM : Mangled model texture package
 * MTD : Texture / model package            (done)
 * MMM : Mangled model package
 * MGL : Mangled texture data
 * MIN : PSX model data
 * FAC : Model faces                        (done)
 * VTX : Model vertices                     (done)
 * NO2 : Model normals                      (done)
 * HIR : Model skeleton                     (done)
 * POM : Mangled map object data
 * POG : Map object data                    (done)
 * PTM : Mangled map textures package
 * PTG : Map textures package               (done)
 * PMM : Mangled terrain data
 * PMG : Terrain data                       (done)
 * OFF : File offset sizes                  (done)
 */

/////////////////////////////////////////////////////////////////////////////////////
// Private

/////////////////////////////////////////////////////////////////////////////////////
// HIR
/////////////////////////////////////////////////////////////////////////////////////

typedef struct HirBone
{
	int32_t parent;
	int16_t coords[ 3 ];
	int8_t unk0[ 10 ];
} HirBone;

typedef enum HirSkeletonBone
{
	PELVIS = 0,
	SPINE,
	HEAD,

	UPPER_ARM_L,
	LOWER_ARM_L,
	HAND_L,

	UPPER_ARM_R,
	LOWER_ARM_R,
	HAND_R,

	UPPER_LEG_L,
	LOWER_LEG_L,
	FOOT_L,

	UPPER_LEG_R,
	LOWER_LEG_R,
	FOOT_R,

	MAX_BONES
} HirSkeletonBone;

static const char *BONE_NAMES[ MAX_BONES ] = {
        "Pelvis",
        "Spine",
        "Head",
        "UpperArm.L",
        "LowerArm.L",
        "Hand.L",
        "UpperArm.R",
        "LowerArm.R",
        "Hand.R",
        "UpperLeg.L",
        "LowerLeg.L",
        "Foot.L",
        "UpperLeg.R",
        "LowerLeg.R",
        "Foot.R",
};

static HirBone *parse_hir( PLFile *file, unsigned int *numBones )
{
	*numBones = ( PlGetFileSize( file ) / sizeof( HirBone ) );
	HirBone *bones = PL_NEW_( HirBone, *numBones );
	if ( PlReadFile( file, bones, sizeof( HirBone ), *numBones ) != *numBones )
	{
		WARN( "Failed to read in all bones for HIR (%s)\n", PlGetFilePath( file ) );
		PL_DELETEN( bones );
	}

	return bones;
}

/////////////////////////////////////////////////////////////////////////////////////
// NO2
/////////////////////////////////////////////////////////////////////////////////////

//TODO: fairly sure these aren't actually stored as floats
typedef struct No2Coord
{
	float v[ 3 ];
	float boneIndex;
} No2Coord;

static No2Coord *parse_no2( PLFile *file, unsigned int *numNormals )
{
	*numNormals = ( PlGetFileSize( file ) / sizeof( No2Coord ) );
	No2Coord *normals = PL_NEW_( No2Coord, *numNormals );
	if ( PlReadFile( file, normals, sizeof( No2Coord ), *numNormals ) != *numNormals )
	{
		WARN( "Failed to read in all normals for NO2 (%s)\n", PlGetFilePath( file ) );
		PL_DELETEN( normals );
	}

	return normals;
}

/////////////////////////////////////////////////////////////////////////////////////
// VTX
/////////////////////////////////////////////////////////////////////////////////////

typedef struct VtxCoord
{
	int16_t v[ 3 ];
	uint16_t boneIndex;
} VtxCoord;

static VtxCoord *parse_vtx( PLFile *file, unsigned int *numVertices )
{
	*numVertices = ( PlGetFileSize( file ) / sizeof( VtxCoord ) );
	VtxCoord *vertices = PL_NEW_( VtxCoord, *numVertices );
	if ( PlReadFile( file, vertices, sizeof( VtxCoord ), *numVertices ) != *numVertices )
	{
		WARN( "Failed to read in all vertices for VTX (%s)\n", PlGetFilePath( file ) );
		PL_DELETEN( vertices );
	}

	return vertices;
}

/////////////////////////////////////////////////////////////////////////////////////
// FAC
/////////////////////////////////////////////////////////////////////////////////////

typedef struct FacQuad
{
	int8_t uvCoords[ 8 ];
	uint16_t vertexIndices[ 4 ];
	uint16_t normalIndices[ 4 ];
	uint32_t textureIndex;
	uint16_t unknown[ 4 ];
} FacQuad;

static FacQuad *parse_quads( PLFile *file, unsigned int numQuads )
{
	bool status = false;
	FacQuad *quads = PL_NEW_( FacQuad, numQuads );
	for ( unsigned int i = 0; i < numQuads; ++i )
	{
		if ( PlReadFile( file, quads[ i ].uvCoords, sizeof( int8_t ), 8 ) != 8 )
		{
			status = false;
			break;
		}
		if ( PlReadFile( file, quads[ i ].vertexIndices, sizeof( uint16_t ), 4 ) != 4 )
		{
			status = false;
			break;
		}
		if ( PlReadFile( file, quads[ i ].normalIndices, sizeof( uint16_t ), 4 ) != 4 )
		{
			status = false;
			break;
		}

		quads[ i ].textureIndex = PL_READUINT32( file, false, &status );
		if ( !status )
		{
			break;
		}

		if ( PlReadFile( file, quads[ i ].unknown, sizeof( uint16_t ), 4 ) != 4 )
		{
			status = false;
			break;
		}

		status = true;
	}

	if ( !status )
	{
		PL_DELETEN( quads );
	}

	return quads;
}

typedef struct FacTriangle
{
	int8_t uvCoords[ 6 ];
	uint16_t vertexIndices[ 3 ];
	uint16_t normalIndices[ 3 ];
	uint16_t unk0;
	uint32_t textureIndex;
	uint16_t unk1[ 4 ];
} FacTriangle;

static FacTriangle *parse_triangles( PLFile *file, unsigned int numTriangles )
{
	bool status = false;
	FacTriangle *triangles = PL_NEW_( FacTriangle, numTriangles );
	for ( unsigned int i = 0; i < numTriangles; ++i )
	{
		if ( PlReadFile( file, triangles[ i ].uvCoords, sizeof( int8_t ), 6 ) != 6 )
		{
			status = false;
			break;
		}
		if ( PlReadFile( file, triangles[ i ].vertexIndices, sizeof( uint16_t ), 3 ) != 3 )
		{
			status = false;
			break;
		}
		if ( PlReadFile( file, triangles[ i ].normalIndices, sizeof( uint16_t ), 3 ) != 3 )
		{
			status = false;
			break;
		}

		triangles[ i ].unk0 = PL_READUINT16( file, false, &status );
		if ( !status )
		{
			break;
		}

		triangles[ i ].textureIndex = PL_READUINT32( file, false, &status );
		if ( !status )
		{
			break;
		}

		if ( PlReadFile( file, triangles[ i ].unk1, sizeof( uint16_t ), 4 ) != 4 )
		{
			status = false;
			break;
		}
	}

	if ( !status )
	{
		PL_DELETEN( triangles );
	}

	return triangles;
}

typedef struct FacTextureIndex
{
	char name[ 16 ];
} FacTextureIndex;

typedef struct FacModel
{
	FacQuad *quads;
	unsigned int numQuads;
	FacTriangle *triangles;
	unsigned int numTriangles;

	FacTextureIndex *textureTable;
	unsigned int textureTableSize;
} FacModel;

static void destroy_fac( FacModel *model )
{
	PL_DELETE( model->triangles );
	PL_DELETE( model->quads );
	PL_DELETE( model );
}

static FacModel *parse_fac( PLFile *file )
{
	if ( !PlFileSeek( file, 16, PL_SEEK_CUR ) )
	{
		WARN( "Failed to seek to data in FAC (%s)\n", PlGetFilePath( file ) );
		return NULL;
	}

	FacModel *model = PL_NEW( FacModel );

	model->numTriangles = PL_READUINT32( file, false, NULL );
	model->triangles = parse_triangles( file, model->numTriangles );
	if ( model->triangles == NULL && model->numTriangles > 0 )
	{
		destroy_fac( model );
		WARN( "Failed to parse triangles in FAC (%s)\n", PlGetFilePath( file ) );
		return NULL;
	}

	model->numQuads = PL_READUINT32( file, false, NULL );
	model->quads = parse_quads( file, model->numQuads );
	if ( model->quads == NULL && model->numQuads > 0 )
	{
		destroy_fac( model );
		WARN( "Failed to parse quads in FAC (%s)\n", PlGetFilePath( file ) );
		return NULL;
	}

	return model;
}

static CookModel *load_fac( const char *path )
{
}

static SSApeFormatModel *conv_fac( const CookModel *model, SSApeFormatModel *out )
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

CookModelFormatInterface modelHowPCInterface = {
        "fac",
        load_fac,
        conv_fac,
        ( CookModelDeleteFunction ) destroy_fac,
};
