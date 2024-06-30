// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_formats.h"
#include "ape/ape_public_model.h"

PL_EXTERN_C

typedef struct PLHashTableNode PLHashTableNode;

typedef struct ApeModelAnimationFrame
{
	PLVector3 mins;
	PLVector3 maxs;
} ApeModelAnimationFrame;

typedef struct ApeModelAnimation
{
	char name[ 64 ];
	ApeModelAnimationFlag flags;

	unsigned int numFrames;

	float speed;

	unsigned int numBones;
} ApeModelAnimation;

typedef struct ApeModel
{
	ApeMaterial *materials[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;

	PLGMesh *meshes[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	unsigned int numMeshes;

	ApeFormatBone bones[ APE_FORMAT_MODEL_MAX_BONES ];
	ApeFormatBone *rootBone;
	unsigned int numBones;

	PLCollisionSphere visSphere;

	PLHashTableNode *node;

	ApeFormatModel disk;

	ApeMemoryReference mem;
} ApeModel;

PL_EXTERN_C_END
