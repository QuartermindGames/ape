// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "model_obj.h"

PL_EXTERN_C

#define SMD_MAX_MESHES    64
#define SMD_MAX_TRIANGLES 4096
#define SMD_MAX_WEIGHTS   4

typedef struct SmdBone
{
	int id;
	char name[ 64 ];
	struct SmdBone *parent;
} SmdBone;

typedef struct SmdWeight
{
	SmdBone *node;
	float value;
} SmdWeight;

typedef struct SmdTriangle
{
	SmdBone *defaultBone;

	PLVector3 position;
	PLVector3 normal;
	PLVector2 uv;

	unsigned int numWeights;
	SmdWeight weights[ SMD_MAX_WEIGHTS ];
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
} SmdModel;

SmdModel *model_smd_load( const char *path );
void model_smd_destroy( SmdModel *model );
bool model_smd_serialize( NdBranch *root, const char *sourcePath );

PL_EXTERN_C_END
