/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <PL/platform_physics.h>

/*
 *  s      s
 *   \      \
 *    a - a  b - b - b
 *   /
 *  b - b - b - b
 */

#define WORLD_VERSION 20210409

#define WORLD_PROP_TAG_LENGTH 64
#define WORLD_PROP_VALUE_LENGTH 256

enum WorldFaceFlag {
	PL_BITFLAG( WORLD_FACE_FLAG_PORTAL, 0U ), /* reflect portal */
	PL_BITFLAG( WORLD_FACE_FLAG_MIRROR, 1U ), /* reflect back */
	PL_BITFLAG( WORLD_FACE_FLAG_SKIP, 2U ),   /* skip face */
};

#define WORLD_FACE_MAX_SIDES 32

typedef struct WorldFace WorldFace;
typedef struct WorldSector WorldSector;
typedef struct World World;

PLVector3 W_GetFaceNormal( const WorldFace *face );
PLVector3 W_GetFaceOrigin( const WorldFace *face );
uint8_t W_GetFaceFlags( const WorldFace *face );
const PLCollisionAABB *W_GetFaceBounds( const WorldFace *face );

WorldFace *W_GetFacesForSector( uint32_t sectorId, uint32_t *numFaces );

const char *W_GetGlobalPropertyValue( const World *world, const char *label );
const char *W_GetSectorPropertyValue( const World *world, unsigned int sectorId, const char *label );

void W_Draw( World *world, PLCamera *camera );
