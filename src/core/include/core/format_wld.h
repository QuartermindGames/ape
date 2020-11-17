/* Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include <stdint.h>

#define WORLD_VERSION   20201116
#define WORLD_MAGIC     "wld"

typedef struct WLDHeader {
	char        magic[ 4 ];
	uint32_t    version;
	uint32_t    createdTime;
	uint32_t    modifiedTime;
	char        author[ 256 ];
} WLDHeader;

typedef struct WLDBrushFace {
	char        texture[ 256 ];
	float       xOffset;
	float       yOffset;
	float       xScale;
	float       yScale;
	float       rotation;
	bool        isPortal;
	struct WLDBrushFace *destination;
} WLDBrushFace;

typedef struct WLDBrush {
	unsigned int numFaces;
	WLDBrushFace *faces;
} WLDBrush;

typedef struct WLDActor {
	char name[ 64 ];
	char *fieldDefs;
	unsigned int size;
	WLDBrush *brushes;
	unsigned int numBrushes;
} WLDActor;

typedef enum WLDNodeType {
	WLD_NODE_ACTOR,
	WLD_NODE_BRUSH,
} WLDNodeType;

typedef struct WLDNode {
	WLDNodeType nodeType;
	union {
		WLDBrush brush;
		WLDActor actor;
	};
} WLDNode;

typedef struct WLDSector {
	WLDNode *children;
	unsigned int numChildren;
} WLDSector;

typedef struct WLDHandle {
	WLDHeader header;

	WLDSector *sectors;
	unsigned int numSectors;
} WLDHandle;
