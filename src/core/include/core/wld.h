/* Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include <stdint.h>

/*
 *  s      s
 *   \      \
 *    a - a  b - b - b
 *   /
 *  b - b - b - b
 */

#define WORLD_VERSION 20201116
#define WORLD_MAGIC "WLD"
#define WORLD_EXTENSION ".wld"

#define WORLD_MAX_PATH 256
#define WORLD_MAX_BRUSH_FACES 16
#define WORLD_MAX_FACE_VERTICES 16

typedef struct WLDHeader {
	char magic[ 4 ];
	uint32_t version;
	uint32_t createdTime;
	uint32_t modifiedTime;
	char author[ 256 ];
} WLDHeader;

enum {
	WLD_FACE_FLAG_PORTAL = ( 1U << 0U ), /* reflect portal */
	WLD_FACE_FLAG_MIRROR = ( 1U << 1U ), /* reflect back */
};

typedef struct WLDBrushFace {
	char material[ WORLD_MAX_PATH ];
	PLVector3 vertices[ WORLD_MAX_FACE_VERTICES ];
	PLVector3 normal;
	float xOffset;
	float yOffset;
	float xScale;
	float yScale;
	float rotation;
	uint32_t flags;
	char tag[ 32 ];
	char targetTag[ 32 ];
} WLDBrushFace;

typedef struct WLDBrush {
	uint32_t numFaces;
	WLDBrushFace faces[ WORLD_MAX_BRUSH_FACES ];
} WLDBrush;

typedef struct WLDActorProperty {
	char name[ 64 ];
	char value[ WORLD_MAX_PATH ];
} WLDActorProperty;

typedef struct WLDActor {
	char name[ 64 ];
	uint16_t numProperties;
	WLDActorProperty *properties;
} WLDActor;

typedef struct WLDVector {
	float x, y, z;
} WLDVector;

typedef struct WLDBounds {
	WLDVector mins, maxs;
} WLDBounds;

typedef struct WLDSector {
	WLDBounds bounds;
} WLDSector;

typedef enum WLDNodeType {
	WLD_NODE_SECTOR,
	WLD_NODE_ACTOR,
	WLD_NODE_BRUSH,
} WLDNodeType;

typedef struct WLDNode {
	WLDNodeType nodeType;
	union {
		WLDSector sector;
		WLDBrush brush;
		WLDActor actor;
	};
	uint32_t numChildren;
	struct WLDNode *children;
} WLDNode;

typedef struct WLDHandle {
	WLDHeader header;
	WLDNode *nodes;
	uint32_t numNodes;
} WLDHandle;

WLDHandle *WLD_LoadFile( const char *path );
void WLD_DestroyHandle( WLDHandle *handle );
