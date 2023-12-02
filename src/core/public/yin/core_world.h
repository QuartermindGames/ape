// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_physics.h>

PL_EXTERN_C

typedef struct NdBranch NdBranch;

/* external elements */
typedef struct SSArlCamera SSArlCamera;
typedef struct SSArlViewport SSArlViewport;
typedef struct SSArlLight SSArlLight;
typedef struct SS_Acl_Entity SS_Acl_Entity;// core_entity.h

/* ======================================================================
 * WORLD INTERFACE
 * ====================================================================*/

typedef struct SSAclWorldFace SSAclWorldFace;
typedef struct SSAclWorldMesh SSAclWorldMesh;
typedef struct ApeWorldObject ApeWorldObject;
typedef struct SSAclWorldRoom SSAclWorldRoom;
typedef struct ApeWorld ApeWorld;

#define SS_ACL_WORLD_VERSION       3
#define SS_ACL_WORLD_EXTENSION     "wld.n"
#define SS_ACL_WORLD_EXTENSION_CFG "wpf.n"

/// Create an entirely new empty world handle.
/// \return New world instance.
ApeWorld *ss_acl_level_create( void );

ApeWorld *ss_acl_level_load( const char *path );

/// Attempts to save the given world to the destination.
/// \param world
/// \param path
/// \return On success, returns true but false otherwise.
bool ss_acl_world_save( ApeWorld *world, const char *path );

void acl_level_destroy( ApeWorld *level );
NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName );

// TODO: move these under the renderer sub-system
void arl_level_draw_wireframe( ApeWorld *world, SSArlCamera *camera );
void arl_level_draw( ApeWorld *world, SSArlCamera *camera, SSArlLight *light, bool ambienceOnly );
void arl_level_draw_stencil_shadows( ApeWorld *world, SSArlCamera *camera, SSArlLight *light );

void ss_acl_level_set_global_defaults( ApeWorld *level );
void ss_acl_level_set_ambience( ApeWorld *world, const PLColourF32 *ambience );
void ss_acl_level_set_clear_colour( ApeWorld *world, const PLColourF32 *colour );
void ss_acl_level_set_fog_colour( ApeWorld *world, const PLColourF32 *colour );

/**
 * Assigning an entity to the world will give the world instance
 * ownership of that entity.
 */
void acl_level_attach_entity( ApeWorld *world, SS_Acl_Entity *entity );

/**
 * Assigning a light to the world will give the world instance
 * ownership of that light.
 */
void ape_level_attach_light( ApeWorld *world, SSArlLight *light );

unsigned int ss_arl_sky_add_layer( const char *path, float scale, float y, float alpha );
void ss_arl_sky_set_layer_alpha( unsigned int slot, float alpha );
void ss_arl_sky_set_layer_offset( unsigned int slot, float x, float y );
void arl_sky_clear_layers( void );
void arl_sky_draw( SSArlCamera *camera );

void acl_level_set_sun_position( ApeWorld *world, const PLVector3 *position );
void ss_acl_level_get_player_start( const ApeWorld *level, PLVector3 *position, PLMatrix3 *orientation );

////////////////////////////////////////////////////////////////////
// Room

#define SS_ACL_WORLD_ROOM_FLAG_COLD    0x2
#define SS_ACL_WORLD_ROOM_FLAG_OUTSIDE 0x4
#define APE_WORLD_ROOM_FLAG_AIRLOCK    0x8
#define APE_WORLD_ROOM_FLAG_AMBIENT    0x20
#define APE_WORLD_ROOM_FLAG_ALPHA      0x40
#define APE_WORLD_ROOM_FLAG_LIFE       0x80
#define APE_WORLD_ROOM_FLAG_PLANKTON   0x1000
#define APE_WORLD_ROOM_FLAG_UNKNOWN0   0x2000
#define APE_WORLD_ROOM_FLAG_SKY        0x40000000

SSAclWorldRoom *ss_acl_level_get_room_at_position( ApeWorld *world, const PLVector3 *position );

////////////////////////////////////////////////////////////////////
// Face

void ss_acl_world_face_generate_bounds( SSAclWorldFace *face );

////////////////////////////////////////////////////////////////////
// Lighting

typedef enum SSArlLightType
{
	APE_LIGHT_TYPE_OMNI,
	APE_LIGHT_TYPE_SPOT,
	APE_LIGHT_TYPE_SUN,

	APE_MAX_LIGHT_TYPES
} SSArlLightType;

// GM flags, do not change!!
#define SS_ARL_LIGHT_FLAG_DYNAMIC         0x1U   // means the light is not baked, and can be moved at runtime
#define SS_ARL_LIGHT_FLAG_FADE            0x2U   // ...
#define SS_ARL_LIGHT_FLAG_SHADOWS         0x4U   // if enabled without runtime shadows flag, will cast lightmap shadows
#define SS_ARL_LIGHT_FLAG_ENABLED         0x8U   // if flag is not present, light is not active
#define SS_ARL_LIGHT_FLAG_RUNTIME_SHADOWS 0x2000U// treated as stencil shadow volumes

/// A light can only be spawned in while the world is active.
/// \param type 	The type of light to be created.
/// \param position Position of the light.
/// \return 		A pointer to the instance of the light. This is owned by the world.
SSArlLight *ss_arl_light_create( const PLVector3 *position, const PLColourF32 *colour, float radius, SSArlLightType type, unsigned int flags );
void ss_arl_light_destroy( SSArlLight *light );

PLColourF32 ss_arl_light_get_colour( const SSArlLight *light );
void ss_arl_light_set_colour( SSArlLight *light, const PLColourF32 *colour );

PLVector3 ss_arl_light_get_position( const SSArlLight *light );
void ss_arl_light_set_position( SSArlLight *light, const PLVector3 *position );

PL_EXTERN_C_END
