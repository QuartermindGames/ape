/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

enum MapFaceFlags {
	PL_BITFLAG( MAP_FLAG_FACE_PORTAL, 0 ), /* face leads into other specified sector */
	PL_BITFLAG( MAP_FLAG_FACE_MIRROR, 1 ), /* face leads into sector flipped */
};

typedef struct MapFace {
	PLPolygon *polygon;
	struct Material *material;
	PLCollisionAABB bounds;
	uint8_t flags;
} MapFace;

typedef struct MapSector {
	MapFace *faces;
	unsigned int numFaces;
} MapSector;

typedef struct GfxCamera GfxCamera;

MapFace *Map_GetFacesForSector( unsigned int sectorNum, unsigned int *numFaces );
void Map_ClearData( void );

void Map_Load( const char *mapName );
void Map_Draw( GfxCamera *camera );

bool Map_CheckCollisions( const PLCollisionAABB *bounds, unsigned int curArea );
