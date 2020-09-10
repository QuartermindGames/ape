/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

enum MapFaceFlags {
	PL_BITFLAG( MAP_FLAG_FACE_PORTAL, 0 ), /* face leads into other specified sector */
	PL_BITFLAG( MAP_FLAG_FACE_MIRROR, 1 ), /* face leads into sector flipped */
};

typedef struct MapVertex {
	float x, y, z; /* vertex position */
	float s, t;    /* these are used for the lightmap coords */
} MapVertex;

typedef struct MapFace {
	PLPolygon *polygon;
	PLCollisionAABB bounds;
	uint8_t flags;
} MapFace;

typedef struct MapSector {
	unsigned int faceId;
} MapSector;

void Map_ClearData( void );
void Map_Load( const char *path );
void Map_Draw( void );

bool Map_CheckCollisions( const PLCollisionAABB *bounds, unsigned int curArea );
