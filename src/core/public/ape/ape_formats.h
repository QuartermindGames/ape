// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

/////////////////////////////////////////////////////////////////////////////////////
// Config Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_CONFIG_EXTENSION "cfg.n"

/////////////////////////////////////////////////////////////////////////////////////
// Effect Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_EFFECT_EXTENSION "eff.n"

/////////////////////////////////////////////////////////////////////////////////////
// Model Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_MODEL_EXTENSION "mdl.n"
#define APE_FORMAT_MODEL_VERSION   2

// much of this is just here for sanity checking -
// in the long-term, we should really look at making
// these dynamically allocated instead...
#define APE_FORMAT_MODEL_MAX_MATERIALS 64
#define APE_FORMAT_MODEL_MAX_BONES     256
#define APE_FORMAT_MODEL_MAX_TRIANGLES 16384
#define APE_FORMAT_MODEL_MAX_VERTICES  ( APE_FORMAT_MODEL_MAX_TRIANGLES * 3 )
#define APE_FORMAT_MODEL_MAX_BONE_NAME 64
#define APE_FORMAT_MODEL_MAX_WEIGHTS   4

typedef enum ApeModelAnimationFlag
{
	PL_BITFLAG( APE_MODEL_ANIMATION_FLAG_LOOPING, 0U ),
} ApeModelAnimationFlag;

typedef struct ApeFormatBone
{
	char name[ APE_FORMAT_MODEL_MAX_BONE_NAME ];
	unsigned int parent;
	PLVector3 rotation;
	PLVector3 position;
} ApeFormatBone;

typedef struct ApeFormatWeight
{
	float weight;
	unsigned int bone;
} ApeFormatWeight;

typedef struct ApeFormatVertex
{
	PLVector3 position;
	PLVector3 normal;
	PLVector2 uv;

	ApeFormatWeight weights[ APE_FORMAT_MODEL_MAX_WEIGHTS ];
	unsigned int numWeights;
} ApeFormatVertex;

typedef struct ApeFormatTriangle
{
	unsigned int indices[ 3 ];
} ApeFormatTriangle;

typedef struct ApeFormatMesh
{
	PLPath material;

	ApeFormatTriangle triangles[ APE_FORMAT_MODEL_MAX_TRIANGLES ];
	unsigned int numTriangles;
} ApeFormatMesh;

typedef struct ApeFormatModel
{
	char name[ 64 ];

	PLPath materialPath;

	ApeFormatVertex vertices[ APE_FORMAT_MODEL_MAX_VERTICES ];
	unsigned int numVertices;

	ApeFormatBone bones[ APE_FORMAT_MODEL_MAX_BONES ];
	unsigned int numBones;

	ApeFormatMesh meshes[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	unsigned int numMeshes;
} ApeFormatModel;

/////////////////////////////////////////////////////////////////////////////////////
// Material Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_MATERIAL_EXTENSION "mat.n"

/////////////////////////////////////////////////////////////////////////////////////
// Texture Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_TEXTURE_MAGIC     PL_MAGIC_TO_NUM( 'A', 'T', 'E', 'X' )
#define APE_FORMAT_TEXTURE_VERSION   1
#define APE_FORMAT_TEXTURE_EXTENSION "tex"

typedef struct ApeFormatTextureIOHeader
{
	uint32_t magic;
	uint32_t version;
} ApeFormatTextureIOHeader;

/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C_END
