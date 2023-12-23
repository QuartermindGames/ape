// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

/////////////////////////////////////////////////////////////////////////////////////
// Config Format
/////////////////////////////////////////////////////////////////////////////////////

#define SS_APE_FORMAT_CONFIG_EXTENSION "cfg.n"

/////////////////////////////////////////////////////////////////////////////////////
// Effect Format
/////////////////////////////////////////////////////////////////////////////////////

#define SS_APE_FORMAT_EFFECT_EXTENSION "eff.n"

/////////////////////////////////////////////////////////////////////////////////////
// Model Format
/////////////////////////////////////////////////////////////////////////////////////

#define SS_APE_FORMAT_MODEL_EXTENSION "mdl.n"
#define SS_APE_FORMAT_MODEL_VERSION   2

#define SS_APE_FORMAT_MODEL_MAX_MATERIALS 64
#define SS_APE_FORMAT_MODEL_MAX_BONES     256
#define SS_APE_FORMAT_MODEL_MAX_TRIANGLES 16384
#define SS_APE_FORMAT_MODEL_MAX_BONE_NAME 64

typedef struct SSApeFormatBone
{
	char name[ SS_APE_FORMAT_MODEL_MAX_BONE_NAME ];
	struct SSApeFormatBone *parent;
} SSApeFormatBone;

typedef struct SSApeFormatVertex
{
	PLVector3 position;
	PLVector3 normal;
	PLVector2 uv;

	unsigned int numWeights;
} SSApeFormatVertex;

typedef struct SSApeFormatTriangle
{
	SSApeFormatVertex vertices[ 3 ];
} SSApeFormatTriangle;

typedef struct SSApeFormatMesh
{
	PLPath material;

	SSApeFormatTriangle triangles[ SS_APE_FORMAT_MODEL_MAX_TRIANGLES ];
	unsigned int numTriangles;
} SSApeFormatMesh;

typedef struct SSApeFormatModel
{
	SSApeFormatBone bones[ SS_APE_FORMAT_MODEL_MAX_BONES ];
	unsigned int numBones;

	SSApeFormatMesh meshes[ SS_APE_FORMAT_MODEL_MAX_MATERIALS ];
	unsigned int numMeshes;
} SSApeFormatModel;

/////////////////////////////////////////////////////////////////////////////////////
// Material Format
/////////////////////////////////////////////////////////////////////////////////////

#define SS_APE_FORMAT_MATERIAL_EXTENSION "mat.n"

/////////////////////////////////////////////////////////////////////////////////////
// Texture Format
/////////////////////////////////////////////////////////////////////////////////////

#define SS_APE_FORMAT_TEXTURE_EXTENSION  "tex.n"

/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C_END
