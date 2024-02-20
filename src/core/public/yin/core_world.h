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
typedef struct ApeRoom ApeRoom;    // world.h

/* ======================================================================
 * WORLD INTERFACE
 * ====================================================================*/

typedef struct ApeWorldNode ApeWorldNode;
typedef struct ApeWorldBrush ApeWorldBrush;

/////////////////////////////////////////////////////////////////////////////////////
// Ape World Node - foundation of all objects associated with the world.
/////////////////////////////////////////////////////////////////////////////////////

typedef enum ApeWorldNodeType
{
	APE_WORLD_NODE_TYPE_EMPTY = 0,
	APE_WORLD_NODE_TYPE_ROOM,// and then brushes, lights, cameras and entities should be attached to this
	APE_WORLD_NODE_TYPE_BRUSH,
	APE_WORLD_NODE_TYPE_LIGHT,
	APE_WORLD_NODE_TYPE_CAMERA,
	APE_WORLD_NODE_TYPE_ENTITY,

	APE_WORLD_MAX_NODE_TYPES
} ApeWorldNodeType;

typedef struct ApeWorldNode
{
	char name[ 64 ];
	ApeWorldNodeType type;

	void *data;

	PLMatrix4 transform;
	PLCollisionAABB bounds;

	ApeWorldNode *parent;
	struct PLLinkedListNode *parentListNode;

	struct PLLinkedList *children;// ApeWorldNode
} ApeWorldNode;

ApeWorldNode *ape_world_node_create( ApeWorldNode *parent, const char *name, ApeWorldNodeType type, void *data );
void ape_world_node_destroy( ApeWorldNode *self );
void ape_world_node_attach_data( ApeWorldNode *self, ApeWorldNodeType type, void *data );

/// Performs some basic validation on the node type before passing back the data.
static inline void *ape_world_node_get_data( ApeWorldNode *self, ApeWorldNodeType expectedType )
{
	if ( self->type != expectedType )
	{
		return NULL;
	}

	return self->data;
}

static inline ApeRoom *ape_world_node_get_room_data( ApeWorldNode *self ) { return ( ApeRoom * ) ape_world_node_get_data( self, APE_WORLD_NODE_TYPE_ROOM ); }
static inline ApeLight *ape_world_node_get_light_data( ApeWorldNode *self ) { return ( ApeLight * ) ape_world_node_get_data( self, APE_WORLD_NODE_TYPE_LIGHT ); }
static inline ApeCamera *ape_world_node_get_camera_data( ApeWorldNode *self ) { return ( ApeCamera * ) ape_world_node_get_data( self, APE_WORLD_NODE_TYPE_CAMERA ); }
static inline ApeEntity *ape_world_node_get_entity_data( ApeWorldNode *self ) { return ( ApeEntity * ) ape_world_node_get_data( self, APE_WORLD_NODE_TYPE_ENTITY ); }

/////////////////////////////////////////////////////////////////////////////////////
// Ape World Brush - the building blocks of the world.
/////////////////////////////////////////////////////////////////////////////////////

typedef enum ApeWorldBrushGeometryType
{
	APE_WORLD_BRUSH_GEOMETRY_TYPE_PRIMITIVE,// class quake-style
	APE_WORLD_BRUSH_GEOMETRY_TYPE_MESH,     // let's you use an explicit mesh instead

	APE_MAX_WORLD_BRUSH_GEOMETRY_TYPES
} ApeWorldBrushGeometryType;

typedef enum ApeWorldBrushType
{
	APE_WORLD_BRUSH_TYPE_SOLID,
	APE_WORLD_BRUSH_TYPE_AIR,

	APE_MAX_WORLD_BRUSH_TYPES
} ApeWorldBrushType;

typedef struct ApeWorldBrushFace
{

} ApeWorldBrushFace;

#define APE_WORLD_BRUSH_MAX_FACES 32

typedef struct ApeWorldBrush
{
	ApeWorldNode node;

	ApeWorldBrushType type;
	ApeWorldBrushGeometryType geometryType;

	struct PLLinkedList *faces;
} ApeWorldBrush;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeWorldFace ApeWorldFace;
typedef struct ApeWorldMesh ApeWorldMesh;
typedef struct ApeWorldObject ApeWorldObject;
typedef struct ApeRoom ApeRoom;

typedef struct PLVectorArray PLVectorArray;
typedef struct PLLinkedList PLLinkedList;

typedef struct ApeWorld
{
	char *name;
	PLPath path;

	PLVectorArray *meshes;

	PLLinkedList *entitySpawns;

	PLVector3 startPosition;
	PLMatrix3 startOrientation;

	ApeWorldNode *root;

	PLVectorArray *materials;// ApeMaterial
	PLVectorArray *rooms;    // ApeWorldRoom
	PLVectorArray *portals;  // ApeWorldPortal
	PLVectorArray *vertices; // ApeWorldVertex
	PLVectorArray *lights;   // ApeLight
	PLVectorArray *entities; // ApeEntity

	PLColourF32 ambience;

	PLColourF32 clearColour;

	PLColourF32 fogColour;
	float fogNear;
	float fogFar;

	PLCollisionAABB bounds;

	/* additional generic properties */
	struct NdBranch *globalProperties;

	uint64_t lastSaveTime;
	bool isDirty;
} ApeWorld;

#define APE_WORLD_VERSION       3
#define APE_WORLD_EXTENSION     "wld.n"
#define APE_WORLD_EXTENSION_CFG "wpf.n"

/// Create an entirely new empty world handle.
/// \return New world instance.
ApeWorld *ape_world_create( void );

ApeWorld *ape_world_load( const char *path );

/// Attempts to save the given world to the destination.
/// \param world
/// \param path
/// \return On success, returns true but false otherwise.
bool ape_world_save( ApeWorld *world, const char *path );

void ape_world_destroy( ApeWorld *level );
NdBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName );

// TODO: move these under the renderer sub-system
void ape_world_draw_wireframe( ApeWorld *world, ApeCamera *camera );
void ape_world_draw( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly );
void ape_world_draw_stencil_shadows( ApeWorld *world, ApeCamera *camera, ApeLight *light );

void ape_world_set_global_defaults( ApeWorld *level );
void ape_world_set_ambience( ApeWorld *world, const PLColourF32 *ambience );
void ape_world_set_clear_colour( ApeWorld *world, const PLColourF32 *colour );
void ape_world_set_fog_colour( ApeWorld *world, const PLColourF32 *colour );

/**
 * Assigning a light to the world will give the world instance
 * ownership of that light.
 */
void ape_world_attach_light( ApeWorld *world, ApeLight *light );

/// Attach a node to the world's root node.
void ape_world_attach_node( ApeWorld *self, ApeWorldNode *node );

unsigned int ape_sky_add_layer( const char *path, float scale, float y, float alpha );
void ape_sky_set_layer_alpha( unsigned int slot, float alpha );
void ape_sky_set_layer_offset( unsigned int slot, float x, float y );
void ape_sky_clear_layers( void );
void ape_sky_draw_( ApeCamera *camera );

////////////////////////////////////////////////////////////////////
// Room

ApeRoom *ape_world_get_room_at_position( ApeWorld *world, const PLVector3 *position );

////////////////////////////////////////////////////////////////////
// Face

void ape_world_face_generate_bounds( ApeWorldFace *face );

////////////////////////////////////////////////////////////////////
// Lighting

typedef enum ApeLightShadowType
{
	SS_APE_LIGHT_SHADOW_TYPE_NONE,
	SS_APE_LIGHT_SHADOW_TYPE_DYNAMIC,
	SS_APE_LIGHT_SHADOW_TYPE_STATIC,

	SS_APE_MAX_LIGHT_SHADOW_TYPES
} ApeLightShadowType;

typedef enum ApeLightType
{
	APE_LIGHT_TYPE_OMNI,
	APE_LIGHT_TYPE_SPOT,
	APE_LIGHT_TYPE_SUN,

	APE_MAX_LIGHT_TYPES
} ApeLightType;

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
ApeLight *ape_light_create( const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, unsigned int flags );
void ape_light_destroy( ApeLight *light );

PLColourF32 ss_arl_light_get_colour( const ApeLight *light );
void ss_arl_light_set_colour( ApeLight *light, const PLColourF32 *colour );

PLVector3 ss_arl_light_get_position( const ApeLight *light );
void ss_arl_light_set_position( ApeLight *light, const PLVector3 *position );

ApeLightShadowType ape_light_get_shadow_type( const ApeLight *light );

PL_EXTERN_C_END
