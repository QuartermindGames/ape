// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_physics.h>

PL_EXTERN_C

/* external elements */
typedef struct ApeCamera ApeCamera;
typedef struct ApeViewport ApeViewport;

/* ======================================================================
 * WORLD INTERFACE
 * ====================================================================*/

typedef struct ApeWorldFace ApeWorldFace;
typedef struct ApeWorldMesh ApeWorldMesh;
typedef struct ApeWorldObject ApeWorldObject;
typedef struct ApeWorldRoom ApeWorldRoom;
typedef struct ApeWorld ApeWorld;

#define APE_WORLD_VERSION        3
#define APE_WORLD_EXTENSION      "wld.n"
#define APE_WORLD_EXTENSION_MESH "wsm.n"

/* World */

ApeWorld *apeCreateWorld( void );
ApeWorld *apeLoadWorld( const char *path );

struct ApeWorld *apeGetCurrentWorld( void );

/**
 * Attempts to save the given world to the destination.
 * On success, returns true but false otherwise.
 */
bool apeSaveWorld( ApeWorld *world, const char *path );

void apeDestroyWorld( ApeWorld *world );
struct NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName );
void apeDrawWorldWireframe_( ApeWorld *world, ApeCamera *camera );
void apeDrawWorld_( ApeWorld *world );
void apeDrawWorldStencilShadowPass_( void );
void apeSetupGlobalWorldDefaults( ApeWorld *world );

ApeWorldRoom *apeGetRoomAtPosition( ApeWorld *world, const PLVector3 *position );

/* Mesh */

ApeWorldMesh *apeCreateWorldMesh( ApeWorld *parent );
ApeWorldMesh *apeLoadWorldMesh( const char *path );
void apeReleaseWorldMesh( ApeWorldMesh *worldMesh );

/* Sector */

struct ApeLight *YnCore_WorldSector_GetVisibleLights( ApeWorldRoom *sector, unsigned int *numLights );

PL_EXTERN_C_END
