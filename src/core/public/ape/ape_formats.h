// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

/////////////////////////////////////////////////////////////////////////////////////
// Config Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_CONFIG_EXTENSION "cfg" ACM_DEFAULT_EXTENSION_OLD

/////////////////////////////////////////////////////////////////////////////////////
// Effect Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_EFFECT_EXTENSION "eff" ACM_DEFAULT_EXTENSION_OLD

/////////////////////////////////////////////////////////////////////////////////////
// Model Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_MODEL_EXTENSION "mdl" ACM_DEFAULT_EXTENSION_OLD
#define APE_FORMAT_MODEL_VERSION   3

// much of this is just here for sanity checking -
// in the long-term, we should really look at making
// these dynamically allocated instead...
#define APE_FORMAT_MODEL_MAX_MATERIALS 16
#define APE_FORMAT_MODEL_MAX_BONES     256
#define APE_FORMAT_MODEL_MAX_TRIANGLES 8192
#define APE_FORMAT_MODEL_MAX_VERTICES  ( APE_FORMAT_MODEL_MAX_TRIANGLES * 3 )
#define APE_FORMAT_MODEL_MAX_BONE_NAME 64
#define APE_FORMAT_MODEL_MAX_WEIGHTS   4

typedef enum ApeModelAnimationFlag
{
	PL_BITFLAG( APE_MODEL_ANIMATION_FLAG_LOOPING, 0U ),
} ApeModelAnimationFlag;

typedef struct ApeFormatWeight
{
	float        weight;
	unsigned int bone;
} ApeFormatWeight;

typedef struct ApeFormatBone
{
	char      name[ APE_FORMAT_MODEL_MAX_BONE_NAME ];
	int       parent;
	PLVector3 rotation;
	PLVector3 position;
} ApeFormatBone;

/////////////////////////////////////////////////////////////////////////////////////
// Material Format
/////////////////////////////////////////////////////////////////////////////////////

#define APE_FORMAT_MATERIAL_EXTENSION "mat" ACM_DEFAULT_EXTENSION_OLD

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
