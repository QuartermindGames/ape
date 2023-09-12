// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_array_vector.h>

#define OBJ_MAX_SUB_OBJECTS 32
#define OBJ_MAX_EDGES       16

typedef struct ObjMaterialLibrary
{

} ObjMaterialLibrary;

typedef struct ObjFace
{
	unsigned int material;
	unsigned int vertices[ OBJ_MAX_EDGES ];
	unsigned int normals[ OBJ_MAX_EDGES ];
	unsigned int textureCoords[ OBJ_MAX_EDGES ];
	unsigned int numEdges;
} ObjFace;

typedef struct ObjSubObject
{
	char name[ 64 ];

	PLVectorArray *vertices;     // PLVector3
	PLVectorArray *normals;      // PLVector3
	PLVectorArray *textureCoords;// PLVector2
	PLVectorArray *faces;        // ObjFace
} ObjSubObject;

typedef struct ObjModel
{
	ObjSubObject subObjects[ OBJ_MAX_SUB_OBJECTS ];
	unsigned int numSubObjects;
} ObjModel;

ObjModel *ObjModel_LoadFromFile( const char *path );
void ObjModel_Destroy( ObjModel *obj );
