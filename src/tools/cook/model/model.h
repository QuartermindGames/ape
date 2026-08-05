// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_formats.h"

#include "model_obj.h"

PL_EXTERN_C

#define SMD_MAX_MESHES    IO_MODEL_MAX_MATERIALS
#define SMD_MAX_TRIANGLES IO_MODEL_MAX_TRIANGLES
#define SMD_MAX_WEIGHTS   4
#define SMD_MAX_BONES     IO_MODEL_MAX_BONES
#define SMD_MAX_FRAMES    2048

typedef struct SmdFrame
{
	int       bone;
	QmMathVector3f position;
	QmMathVector3f rotation;
} SmdFrame;

typedef struct SmdBone
{
	int             id;
	char            name[ IO_MODEL_MAX_BONE_NAME ];
	struct SmdBone *parent;
	SmdFrame        frames[ SMD_MAX_FRAMES ];
} SmdBone;

typedef struct SmdWeight
{
	SmdBone *node;
	float    value;
} SmdWeight;

typedef struct SmdVertex
{
	SmdBone *defaultBone;

	QmMathVector3f position;
	QmMathVector3f normal;
	QmMathVector2f uv;

	unsigned int numWeights;
	SmdWeight    weights[ SMD_MAX_WEIGHTS ];
} SmdVertex;

typedef struct SmdTriangle
{
	SmdVertex vertices[ 3 ];
} SmdTriangle;

typedef struct SmdMesh
{
	PLPath material;

	SmdTriangle  triangles[ SMD_MAX_TRIANGLES ];
	unsigned int numTriangles;
} SmdMesh;

typedef struct SmdModel
{
	SmdMesh      meshes[ SMD_MAX_MESHES ];
	unsigned int numMeshes;

	SmdBone      bones[ SMD_MAX_BONES ];
	unsigned int numBones;
} SmdModel;

SmdModel *model_smd_load( const char *path );
void      model_smd_destroy( SmdModel *model );

/////////////////////////////////////////////////////////////

typedef struct CookModelVertex
{
	QmMathVector3f position;
	QmMathVector3f normal;
	QmMathVector2f uv;

	IOModelVertexWeight weights[ IO_MODEL_MAX_WEIGHTS ];
	unsigned int    numWeights;
} CookModelVertex;

typedef struct CookModelTriangle
{
	unsigned int indices[ 3 ];
} CookModelTriangle;

typedef struct CookModelMesh
{
	PLPath material;

	CookModelTriangle triangles[ IO_MODEL_MAX_TRIANGLES ];
	unsigned int      numTriangles;
} CookModelMesh;

typedef struct CookModel
{
	char name[ 64 ];

	PLPath materialPath;

	CookModelVertex vertices[ IO_MODEL_MAX_VERTICES ];
	unsigned int    numVertices;

	IOModelBone bones[ IO_MODEL_MAX_BONES ];
	unsigned int  numBones;

	CookModelMesh meshes[ IO_MODEL_MAX_MATERIALS ];
	unsigned int  numMeshes;

	float scale;

	bool isStatic;
} CookModel;

PL_EXTERN_C_END
