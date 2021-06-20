/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

#include <plcore/pl_physics.h>

#define WORLD_VERSION 20210409

#define WORLD_PROP_TAG_LENGTH   64
#define WORLD_PROP_VALUE_LENGTH 256

enum WorldFaceFlag
{
	PL_BITFLAG( WORLD_FACE_FLAG_PORTAL, 0U ), /* reflect portal */
	PL_BITFLAG( WORLD_FACE_FLAG_MIRROR, 1U ), /* reflect back */
	PL_BITFLAG( WORLD_FACE_FLAG_SKIP, 2U ),   /* skip face */
};

typedef enum WorldObjectCollisionType
{
	WORLD_OBJECT_COLLISION_POLY,
	WORLD_OBJECT_COLLISION_SPHERE,
	WORLD_OBJECT_COLLISION_AABB,
} WorldObjectCollisionType;

#define WORLD_FACE_MAX_SIDES 32

typedef struct WorldFace   WorldFace;
typedef struct WorldMesh   WorldMesh;
typedef struct WorldObject WorldObject;
typedef struct WorldSector WorldSector;
typedef struct World       World;

/* World */

World *        W_LoadWorld( const char *path );
void           W_DestroyWorld( World *world );
struct NLNode *W_GetWorldProperty( World *world, const char *propertyName );
PLVector4      W_GetAmbience( World *world );
PLVector4      W_GetSunColour( World *world );
PLVector3      W_GetSunPosition( World *world );

/* WorldMesh */

WorldMesh *W_LoadWorldMesh( const char *path );
void       W_ReleaseWorldMesh( WorldMesh *worldMesh );

/* WorldFace */

PLVector3              W_GetFaceNormal( const WorldFace *face );
PLVector3              W_GetFaceOrigin( const WorldFace *face );
uint8_t                W_GetFaceFlags( const WorldFace *face );
const PLCollisionAABB *W_GetFaceBounds( const WorldFace *face );

WorldMesh *W_GetMeshForSector( WorldSector *sector );
WorldFace *W_GetFacesForSector( WorldSector *sector, uint32_t *numFaces );

void W_Draw( PLGCamera *camera, World *world, WorldSector *originSector );
