// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "ape/ape_formats.h"

#include "model_obj.h"

PL_EXTERN_C

#define SMD_MAX_MESHES    APE_FORMAT_MODEL_MAX_MATERIALS
#define SMD_MAX_TRIANGLES APE_FORMAT_MODEL_MAX_TRIANGLES
#define SMD_MAX_WEIGHTS   4
#define SMD_MAX_BONES     APE_FORMAT_MODEL_MAX_BONES

typedef struct SmdBone
{
	int id;
	char name[ APE_FORMAT_MODEL_MAX_BONE_NAME ];
	struct SmdBone *parent;
} SmdBone;

typedef struct SmdWeight
{
	SmdBone *node;
	float value;
} SmdWeight;

typedef struct SmdVertex
{
	SmdBone *defaultBone;

	PLVector3 position;
	PLVector3 normal;
	PLVector2 uv;

	unsigned int numWeights;
	SmdWeight weights[ SMD_MAX_WEIGHTS ];
} SmdVertex;

typedef struct SmdTriangle
{
	SmdVertex vertices[ 3 ];
} SmdTriangle;

typedef struct SmdMesh
{
	PLPath material;

	SmdTriangle triangles[ SMD_MAX_TRIANGLES ];
	unsigned int numTriangles;
} SmdMesh;

typedef struct SmdModel
{
	SmdMesh meshes[ SMD_MAX_MESHES ];
	unsigned int numMeshes;

	SmdBone bones[ SMD_MAX_BONES ];
	unsigned int numBones;
} SmdModel;

SmdModel *model_smd_load( const char *path );
void model_smd_destroy( SmdModel *model );

PL_EXTERN_C_END
