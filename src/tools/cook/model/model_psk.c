// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "../cook.h"

#include "model.h"

typedef struct PskChunkHeader
{
	char     id[ 20 ];
	uint32_t typeFlags;
	uint32_t dataSize;
	uint32_t dataCount;
} PskChunkHeader;

typedef struct PskPoint
{
	PLVector3 point;
} PskPoint;

typedef struct PskVertex
{
	uint16_t pointIndex;
	float    u;
	float    v;
	uint8_t  materialIndex;
	uint8_t  unused;
} PskVertex;

typedef struct PskTriangle
{
	uint16_t wedgeIndex[ 3 ];
	uint8_t  materialIndex;
	uint8_t  auxMaterialIndex;
	uint32_t smoothingGroup;
} PskTriangle;

typedef struct PskMaterial
{
	char     name[ 64 ];
	uint32_t textureIndex;
	uint32_t polyFlags;
	uint32_t unused[ 4 ];
} PskMaterial;

typedef struct PskModel
{
} PskModel;
