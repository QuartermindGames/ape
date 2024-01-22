// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

#include "ape/ape_formats.h"

typedef struct PLHashTableNode PLHashTableNode;

typedef enum SSApeModelAnimationFlag
{
	PL_BITFLAG( SS_APE_MODEL_ANIMATION_FLAG_LOOPING, 0U ),
} SSApeModelAnimationFlag;

typedef struct SSApeModelAnimationFrame
{
	PLVector3 mins;
	PLVector3 maxs;
} SSApeModelAnimationFrame;

typedef struct SSApeModelAnimation
{
	char name[ 64 ];
	SSApeModelAnimationFlag flags;

	unsigned int numFrames;

	float speed;

	unsigned int numBones;
} SSApeModelAnimation;

typedef struct SSApeModelBone
{
	char name[ 64 ];
	unsigned int parent;
	PLVector3 position;
	PLQuaternion orientation;
} SSApeModelBone;

typedef struct SSApeModel
{
	ApeMaterial *materials[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;

	PLGMesh *meshes[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	unsigned int numMeshes;

	SSApeModelBone bones[ APE_FORMAT_MODEL_MAX_BONES ];
	SSApeModelBone *rootBone;
	unsigned int numBones;

	PLCollisionSphere visSphere;

	PLHashTableNode *node;

	ApeMemoryReference mem;
} SSApeModel;

SSApeModel *ss_ape_model_load( const char *path );
void ss_ape_model_release( SSApeModel *model );

void ss_ape_model_draw( SSApeModel *model );

PL_EXTERN_C_END
