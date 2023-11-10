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
typedef struct ApeEntity ApeEntity;// core_entity.h

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
ApeWorld *ape_world_create( void );

ApeWorld *apeLoadWorld( const char *path );

/// Deserialize world from a node tree.
/// \param world World that deserialized data will be added to.
/// \param root Handle to the world root.
/// \return On success, returns the world pointer, otherwise null.
ApeWorld *acl_world_deserialize( ApeWorld *world, NdBranch *root );

/// Fetches the currently active world. Only one world can be active at a time.
/// \return Handle to the currently active world.
struct ApeWorld *acl_world_get_current( void );

/// Attempts to save the given world to the destination.
/// \param world
/// \param path
/// \return On success, returns true but false otherwise.
bool apeSaveWorld( ApeWorld *world, const char *path );

void ape_world_destroy( ApeWorld *world );
NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName );
void apeDrawWorldWireframe_( ApeWorld *world, ApeCamera *camera );
void apeDrawWorld_( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly );
void apeDrawWorldStencilShadowPass_( ApeWorld *world, ApeCamera *camera, ApeLight *light );

void acl_world_set_global_defaults( ApeWorld *world );
void acl_world_set_ambience( ApeWorld *world, const PLColourF32 *ambience );
void acl_world_set_clear_colour( ApeWorld *world, const PLColourF32 *colour );
void acl_level_set_fog_colour( ApeWorld *world, const PLColourF32 *colour );

/**
 * Assigning an entity to the world will give the world instance
 * ownership of that entity.
 */
void ape_world_attach_entity( ApeWorld *world, ApeEntity *entity );

/**
 * Assigning a light to the world will give the world instance
 * ownership of that light.
 */
void ape_world_attach_light( ApeWorld *world, ApeLight *light );

unsigned int arl_sky_add_layer( const char *path, float scale, float y, float speed, float alpha );
void arl_sky_set_layer_alpha( unsigned int slot, float alpha );
void arl_sky_clear_layers( void );
void arl_sky_draw( ApeCamera *camera );

void apeSetSunPosition( ApeWorld *world, const PLVector3 *position );

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

////////////////////////////////////////////////////////////////////
// Lighting

typedef enum ApeLightType
{
	APE_LIGHT_TYPE_OMNI,
	APE_LIGHT_TYPE_SPOT,
	APE_LIGHT_TYPE_SUN,

	APE_MAX_LIGHT_TYPES
} ApeLightType;

// GM flags, do not change!!
#define APE_LIGHT_FLAG_DYNAMIC         0x1U
#define APE_LIGHT_FLAG_FADE            0x2U
#define APE_LIGHT_FLAG_SHADOWS         0x4U
#define APE_LIGHT_FLAG_ENABLED         0x8U
#define APE_LIGHT_FLAG_RUNTIME_SHADOWS 0x2000U

/// A light can only be spawned in while the world is active.
/// \param type 	The type of light to be created.
/// \param position Position of the light.
/// \return 		A pointer to the instance of the light. This is owned by the world.
ApeLight *ape_light_create( const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, unsigned int flags );
void ape_light_destroy( ApeLight *light );

PLColourF32 ape_light_get_colour( const ApeLight *light );
void ape_light_set_colour( ApeLight *light, const PLColourF32 *colour );

PLVector3 ape_light_get_position( const ApeLight *light );
void ape_light_set_position( ApeLight *light, const PLVector3 *position );

PL_EXTERN_C_END
