/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include <stdint.h>

typedef struct BSPVector3f {
	float x, y, z;
} BSPVector3f;

typedef struct BSPVector3i {
	int16_t x, y, z;
} BSPVector3i;

typedef struct BSPLump {
	uint32_t offset;
	uint32_t length;
} BSPLump;

enum BSPLumpType {
	BSP_LUMP_ENTITIES,
	BSP_LUMP_PLANES,
	BSP_LUMP_VERTICES,
	BSP_LUMP_VISIBILITY,
	BSP_LUMP_NODES,
	BSP_LUMP_TEXTURES,
	BSP_LUMP_FACES,
	BSP_LUMP_LIGHTMAPS,
	BSP_LUMP_LEAVES,
	BSP_LUMP_FACE_TABLE,
	BSP_LUMP_BRUSH_TABLE,
	BSP_LUMP_EDGES,
	BSP_LUMP_EDGE_TABLE,
	BSP_LUMP_MODELS,
	BSP_LUMP_BRUSHES,
	BSP_LUMP_BRUSH_SIDES,
	BSP_LUMP_POP,
	BSP_LUMP_AREAS,
	BSP_LUMP_AREA_PORTALS,

	BSP_MAX_LUMPS
};

typedef struct BSPHeader {
	char magic[ 4 ];
	uint32_t version;
	BSPLump lumps[ BSP_MAX_LUMPS ];
} BSPHeader;

typedef struct BSPPlane {
	BSPVector3f normal;
	float distance;
	uint32_t type;
} BSPPlane;

typedef struct BSPNode {
	uint32_t plane;
	int32_t frontChild;
	int32_t backChild;
	BSPVector3i mins;
	BSPVector3i maxs;
	uint16_t firstFace;
	uint16_t numFaces;
} BSPNode;

typedef struct BSPTexture {
	BSPVector3f uAxis;
	float uOffset;
	BSPVector3f vAxis;
	float vOffset;
	uint32_t flags;
	uint32_t value;
	char textureName[ 32 ];
	uint32_t nextTexture;
} BSPTexture;

typedef struct BSPFace {
	uint16_t plane;
	uint16_t planeSide;
	uint32_t firstEdge;
	uint16_t numEdges;
	uint16_t textureInfo;
	uint8_t lightmapStyles[ 4 ];
	uint32_t lightmapOffset;
} BSPFace;

typedef struct BSPLeaf {
	uint32_t brushOr;
	uint16_t cluster;
	uint16_t area;
	BSPVector3i mins;
	BSPVector3i maxs;
	uint16_t firstLeafFace;
	uint16_t numLeafFaces;
	uint16_t firstLeafBrush;
	uint16_t numLeafBrushes;
} BSPLeaf;

typedef struct BSPHandle {
	BSPVector3f *vertices;
	unsigned int numVertices;
	BSPFace *faces;
	unsigned int numFaces;
	BSPTexture *textures;
	unsigned int numTextures;
	BSPLeaf *leaves;
	unsigned int numLeaves;
} BSPHandle;
