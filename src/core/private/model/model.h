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
	char                  name[ 64 ];
	ApeModelAnimationFlag flags;

	uint numFrames;

	float speed;

	uint numBones;
} ApeModelAnimation;

typedef struct ApeModel
{
	PLGMesh     *meshes[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	ApeMaterial *materials[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	uint         numMaterials;

	ApeFormatBone  bones[ APE_FORMAT_MODEL_MAX_BONES ];
	ApeFormatBone *rootBone;
	uint           numBones;

	PLCollisionSphere visSphere;

	ApeFormatModel disk;

	ApeMemoryReference reference;
} ApeModel;

PL_EXTERN_C_END
