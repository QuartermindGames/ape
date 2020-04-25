/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

typedef struct MapVertex {
	int32_t x, y, z;
} MapVertex;

typedef struct MapPolygon {
	PLPolygon   *polygon;
	PLTexture   *texture;
	PLVector2   textureOffset;
	PLVector2   textureScale;
	PLVector2   normal;
	uint8_t     flags;
} MapFace;

typedef struct MapSector {
	unsigned int numLines;
	unsigned int *lineIndices;
	int          max[ 2 ]; /* boundary maximum */
	int          min[ 2 ]; /* boundary minimum */
} MapSector;

void Map_Load( const char *path );
void Map_Draw( void );

bool Map_CheckCollisions( const PLCollisionAABB *bounds, unsigned int curArea );
