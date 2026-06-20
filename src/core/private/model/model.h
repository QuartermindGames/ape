// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_formats.h"
#include "ape/ape_public_model.h"

PL_EXTERN_C

typedef struct PLHashTableNode PLHashTableNode;

typedef struct ApeModelAnimationFrame
{
	QmMathVector3f mins;
	QmMathVector3f maxs;
} ApeModelAnimationFrame;

typedef struct ApeModelAnimation
{
	char                  name[ 64 ];
	ApeModelAnimationFlag flags;

	unsigned int numFrames;

	float speed;

	unsigned int numBones;
} ApeModelAnimation;

typedef struct ApeModelVertexWeight
{
	ApeFormatWeight weights[ APE_FORMAT_MODEL_MAX_WEIGHTS ];
	unsigned int    numWeights;
} ApeModelVertexWeight;

typedef struct ApeModelMesh
{
	ApeMaterial         *material;
	ApeModelVertexWeight weights[ APE_FORMAT_MODEL_MAX_VERTICES ];

	unsigned int startIndex;
	unsigned int endIndex;
} ApeModelMesh;

typedef enum ApeModelFlag
{
	QM_OS_BIT_FLAG( APE_MODEL_FLAG_ANIMATED, 0 ),// if the model doesn't have this, it's assumed static
} ApeModelFlag;

typedef struct ApeModel
{
	ApeModelMesh meshes[ APE_FORMAT_MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;// also corrisponds to number of meshes...

	ApeFormatBone  bones[ APE_FORMAT_MODEL_MAX_BONES ];
	ApeFormatBone *rootBone;
	unsigned int   numBones;

	unsigned int flags;

	PLGMesh *cache;

	PLLinkedList *sceneNodes;

	ApeMemoryReference reference;
} ApeModel;

typedef struct ApeModelNode
{
	// This should always come first!
	ApeWorldNode base;

	PLPath    modelPath;
	ApeModel *model;

	struct PLLinkedListNode *modelSceneNode;
} ApeModelNode;

PL_EXTERN_C_END
