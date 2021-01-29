/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <stdint.h>

/*
 *  s      s
 *   \      \
 *    a - a  b - b - b
 *   /
 *  b - b - b - b
 */

#define WORLD_VERSION 20201122
#define WORLD_MAGIC "WLD"
#define WORLD_EXTENSION ".wld"

#define WORLD_MAX_PATH 256
#define WORLD_MAX_TAG 64
#define WORLD_MAX_BRUSH_FACES 16
#define WORLD_MAX_FACE_VERTICES 16

typedef struct WldHeader {
	char magic[ 4 ];
	uint32_t version;
} WldHeader;

enum {
	WLD_FACE_FLAG_PORTAL = ( 1U << 0U ), /* reflect portal */
	WLD_FACE_FLAG_MIRROR = ( 1U << 1U ), /* reflect back */
	WLD_FACE_FLAG_SKIP = ( 1U << 2U ), /* skip face */
};

typedef char WldString[ WORLD_MAX_PATH ];
typedef char WldTag[ WORLD_MAX_TAG ];

typedef struct WldVector { float x, y, z; } WldVector;
typedef struct WldVector4 { float x, y, z, w; } WldVector4;

typedef struct WldBrushFace {
	WldString material;
	WldVector vertices[ WORLD_MAX_FACE_VERTICES ];
	WldVector normal;
	WldVector4 tm[ 2 ];
	float scaleX, scaleY;
	float rotation;
	uint32_t flags; /* flags, e.g. is portal, skip the face etc. */
	WldTag tag; /* face tag, for portals */
	WldTag targetTag; /* target tag, for portals */
} WldBrushFace;

typedef struct wldBrush {
	uint32_t numFaces;
	WldBrushFace faces[ WORLD_MAX_BRUSH_FACES ];
} wldBrush;

typedef struct WldProperty {
	WldTag name;
	WldString value;
} WldProperty;

typedef struct WLDEntity {
	uint16_t numProperties;
	WldProperty *properties;
} WldEntity;

typedef struct WldSector {
	WldTag name;
} WldSector;

typedef enum WldNodeType {
	WLD_NODE_SECTOR,
	WLD_NODE_ENTITY,
	WLD_NODE_BRUSH,
} WldNodeType;

typedef struct WldNode {
	WldTag name;
	WldNodeType nodeType;
	union {
		WldSector sector;
		wldBrush brush;
		WldEntity entity;
	};
	uint32_t numChildren;
	struct WldNode *children;
} WldNode;

typedef struct WldHandle {
	WldHeader header;
	uint16_t numProperties;
	WldProperty *properties;
	uint32_t numRooms;
	WldNode *rooms;
} WldHandle;

PL_EXTERN_C

PL_EXTERN WldHandle *WLD_LoadFile( const char *path );
PL_EXTERN void WLD_DestroyHandle( WldHandle *handle );

PL_EXTERN_C_END
