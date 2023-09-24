// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_physics.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

/* external elements */
typedef struct ApeCamera ApeCamera;
typedef struct ApeViewport ApeViewport;
typedef struct ApeLight ApeLight;

/* ======================================================================
 * WORLD INTERFACE
 * ====================================================================*/

typedef struct ApeWorldFace ApeWorldFace;
typedef struct ApeWorldMesh ApeWorldMesh;
typedef struct ApeWorldObject ApeWorldObject;
typedef struct ApeWorldRoom ApeWorldRoom;
typedef struct ApeWorld ApeWorld;

#define APE_WORLD_VERSION       3
#define APE_WORLD_EXTENSION     "wld.n"
#define APE_WORLD_EXTENSION_GEO "wgf.n"
#define APE_WORLD_EXTENSION_ENT "wef.n"
#define APE_WORLD_EXTENSION_LIT "wlf.n"
#define APE_WORLD_EXTENSION_CFG "wpf.n"

/// Create an entirely new empty world handle.
/// \return New world instance.
ApeWorld *apeCreateWorld( void );

ApeWorld *apeLoadWorld( const char *path );

/// Deserialize world from a node tree.
/// \param world World that deserialized data will be added to.
/// \param root Handle to the world root.
/// \return On success, returns the world pointer, otherwise null.
ApeWorld *apeDeserializeWorld( ApeWorld *world, NdBranch *root );

/// Fetches the currently active world. Only one world can be active at a time.
/// \return Handle to the currently active world.
struct ApeWorld *apeGetCurrentWorld( void );

/// Attempts to save the given world to the destination.
/// \param world
/// \param path
/// \return On success, returns true but false otherwise.
bool apeSaveWorld( ApeWorld *world, const char *path );

void apeDestroyWorld( ApeWorld *world );
NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName );
void apeDrawWorldWireframe_( ApeWorld *world, ApeCamera *camera );
void apeDrawWorld_( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly );
void apeDrawWorldStencilShadowPass_( ApeWorld *world, ApeCamera *camera, ApeLight *light );
void apeSetupGlobalWorldDefaults( ApeWorld *world );

void apeAddSkyLayer_( const char *path );
void apeClearSkyLayers_( void );
void apeDrawSky_( ApeCamera *camera );

void apeGetPlayerStart( const ApeWorld *world, PLVector3 *position, PLMatrix3 *orientation );

/* Mesh */

ApeWorldMesh *apeCreateWorldMesh( ApeWorld *parent );

////////////////////////////////////////////////////////////////////
// Room

#define APE_WORLD_ROOM_FLAG_COLD     0x2
#define APE_WORLD_ROOM_FLAG_OUTSIDE  0x4
#define APE_WORLD_ROOM_FLAG_AIRLOCK  0x8
#define APE_WORLD_ROOM_FLAG_AMBIENT  0x20
#define APE_WORLD_ROOM_FLAG_ALPHA    0x40
#define APE_WORLD_ROOM_FLAG_LIFE     0x80
#define APE_WORLD_ROOM_FLAG_PLANKTON 0x1000
#define APE_WORLD_ROOM_FLAG_UNKNOWN0 0x2000
#define APE_WORLD_ROOM_FLAG_SKY      0x40000000

ApeWorldRoom *apeGetRoomAtPosition( ApeWorld *world, const PLVector3 *position );

////////////////////////////////////////////////////////////////////
// Face

void apeGenerateWorldFaceBounds( ApeWorldFace *face );

PL_EXTERN_C_END
