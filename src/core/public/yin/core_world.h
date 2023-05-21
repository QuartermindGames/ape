// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_physics.h>

PL_EXTERN_C

/* external elements */
typedef struct OgeCamera OgeCamera;
typedef struct OgeViewport OgeViewport;

/* ======================================================================
 * WORLD INTERFACE
 * ====================================================================*/

typedef struct OgeWorldFace OgeWorldFace;
typedef struct OgeWorldMesh OgeWorldMesh;
typedef struct OgeWorldObject OgeWorldObject;
typedef struct OgeWorldSector OgeWorldSector;
typedef struct OgeWorld OgeWorld;

#define YN_CORE_WORLD_VERSION 2

#define YN_CORE_WORLD_EXTENSION      "wld.n"
#define YN_CORE_WORLD_EXTENSION_MESH "wsm.n"

/* World */

OgeWorld *YnCore_World_Create( void );
OgeWorld *YnCore_World_Load( const char *path );

/**
 * Attempts to save the given world to the destination.
 * On success, returns true but false otherwise.
 */
bool YnCore_World_Save( OgeWorld *world, const char *path );

void YnCore_World_Destroy( OgeWorld *world );
struct NdBranch *YnCore_World_GetProperty( OgeWorld *world, const char *propertyName );
PLColourF32 YnCore_World_GetAmbience( OgeWorld *world );
PLColourF32 YnCore_World_GetSunColour( OgeWorld *world );
PLVector3 YnCore_World_GetSunPosition( OgeWorld *world );
void YnCore_World_DrawWireframe( OgeWorld *world, OgeCamera *camera );
void YnCore_World_Draw( OgeWorld *world, OgeWorldSector *originSector, OgeCamera *camera );
void YnCore_World_SetupGlobalDefaults( OgeWorld *world );

uint64_t YnCore_World_GetLastSaveTime( const OgeWorld *world );

OgeWorldSector *YnCore_World_GetSectorByGlobalOrigin( OgeWorld *world, const PLVector3 *globalOrigin );

const char *YnCore_World_GetPath( const OgeWorld *world );

/* Mesh */

OgeWorldMesh *YnCore_WorldMesh_Create( OgeWorld *parent );
OgeWorldMesh *YnCore_WorldMesh_Load( const char *path );
void YnCore_WorldMesh_Release( OgeWorldMesh *worldMesh );

/* Face */

PLVector3 YnCore_WorldFace_GetNormal( const OgeWorldFace *face );
PLVector3 YnCore_WorldFace_GetOrigin( const OgeWorldFace *face );
uint8_t YnCore_WorldFace_GetFlags( const OgeWorldFace *face );
const PLCollisionAABB *YnCore_WorldFace_GetBounds( const OgeWorldFace *face );

/* Sector */

struct OgeLight *YnCore_WorldSector_GetVisibleLights( OgeWorldSector *sector, unsigned int *numLights );
OgeWorldMesh *YnCore_WorldSector_GetMesh( OgeWorldSector *sector );
OgeWorldFace **YnCore_WorldSector_GetMeshFaces( OgeWorldSector *sector, uint32_t *numFaces );

PL_EXTERN_C_END
