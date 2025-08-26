// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_array_vector.h>
#include "ape/ape_formats.h"

PL_EXTERN_C

#define OBJ_MAX_SUB_OBJECTS 32
#define OBJ_MAX_EDGES       16
#define OBJ_MAX_MATERIALS   64

typedef struct ObjVertex
{
	QmMathVector3f position;
	QmMathVector3f colour;
} ObjVertex;

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
	QmMathVector3f    normal;
} ObjFace;

typedef struct ObjSubObject
{
	char name[ 64 ];

	PLVectorArray *faces;// ObjFace

	QmMathVector3f mins, maxs;// bounds
} ObjSubObject;

typedef struct ObjModel
{
	bool storesColour;

	PLVectorArray *vertices;     // ObjVertex
	PLVectorArray *normals;      // QmMathVector3f
	PLVectorArray *textureCoords;// QmMathVector2f

	ObjMaterial  materials[ OBJ_MAX_MATERIALS ];
	unsigned int numMaterials;

	ObjSubObject subObjects[ OBJ_MAX_SUB_OBJECTS ];
	unsigned int numSubObjects;
} ObjModel;

ObjModel *model_obj_load( const char *path );
void      model_obj_destroy( ObjModel *obj );

CookModel *model_obj_to_ape( const ObjModel *obj, CookModel *out );

PL_EXTERN_C_END
