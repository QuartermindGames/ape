// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_formats.h"
#include "ape/ape_public_model.h"

#include "renderer/renderer.h"

PL_EXTERN_C

typedef struct ApeModelMesh
{
	ApeMaterial *material;

	unsigned int startIndex;
	unsigned int endIndex;
} ApeModelMesh;

typedef struct ApeModel
{
	ApeModelMesh meshes[ IO_MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;// also corrisponds to number of meshes...

	IOModelBone  bones[ IO_MODEL_MAX_BONES ];
	IOModelBone *rootBone;
	unsigned int numBones;

	unsigned int flags;

	QmGfxMesh *cache;

	PLLinkedList *sceneNodes;

	ApeMemoryReference reference;
} ApeModel;

typedef struct ApeModelNode
{
	// This should always come first!
	ApeWorldNode base;

	PLPath    modelPath;
	ApeModel *model;

	ApeRendererLightGridSample light;

	struct PLLinkedListNode *modelSceneNode;
} ApeModelNode;

PL_EXTERN_C_END
