// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_array_vector.h>

PL_EXTERN_C

#define OBJ_MAX_SUB_OBJECTS 32
#define OBJ_MAX_EDGES       16
#define OBJ_MAX_MATERIALS   64

typedef struct ObjMaterial
{
	char name[ 64 ];

	PLPath diffuseMap;
	PLPath specularMap;
	PLPath ambienceMap;
	PLPath normalMap;
} ObjMaterial;

typedef enum ObjIndex
{
	OBJ_INDEX_VERTEX,
	OBJ_INDEX_TEXTURE,
	OBJ_INDEX_NORMAL,
	OBJ_MAX_INDEXES
} ObjIndex;

typedef struct ObjFace
{
	unsigned int material;
	unsigned int smoothingGroup;

	// these are all explicit indices into the vertices, normals etc.
	unsigned int indices[ OBJ_MAX_EDGES ][ OBJ_MAX_INDEXES ];

	unsigned int numEdges;
	PLVector3 normal;
} ObjFace;

typedef struct ObjSubObject
{
	char name[ 64 ];

	PLVectorArray *faces;// ObjFace

	PLVector3 mins, maxs;// bounds
} ObjSubObject;

typedef struct ObjModel
{
	PLVectorArray *vertices;     // PLVector3
	PLVectorArray *normals;      // PLVector3
	PLVectorArray *textureCoords;// PLVector2

	ObjMaterial materials[ OBJ_MAX_MATERIALS ];
	unsigned int numMaterials;

	ObjSubObject subObjects[ OBJ_MAX_SUB_OBJECTS ];
	unsigned int numSubObjects;
} ObjModel;

ObjModel *model_obj_load( const char *path );
void model_obj_destroy( ObjModel *obj );
bool model_obj_serialize( NdBranch *root, const char *sourcePath );

PL_EXTERN_C_END
