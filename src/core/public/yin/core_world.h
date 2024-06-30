// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_physics.h>

PL_EXTERN_C

typedef struct PLVectorArray PLVectorArray;
typedef struct PLLinkedList PLLinkedList;

typedef struct NdBranch NdBranch;

/* external elements */
typedef struct ApeMaterial ApeMaterial;
typedef struct ApeCamera ApeCamera;
typedef struct ApeViewport ApeViewport;
typedef struct ApeLight ApeLight;
typedef struct ApeEntity ApeEntity;// core_entity.h
typedef struct ApeRoom ApeRoom;    // world.h
typedef struct ApeBrush ApeBrush;
typedef struct ApeWorld ApeWorld;

/* ======================================================================
 * WORLD INTERFACE
 * ====================================================================*/

typedef struct ApeWorldNode ApeWorldNode;

/////////////////////////////////////////////////////////////////////////////////////
// Ape World Node - foundation of all objects associated with the world.
// In hindsight, this should have been a header for objects that can be used by the
// node tree, but alas, I only thought about that after the fact. Maybe in the future
// I'll look into it again.
/////////////////////////////////////////////////////////////////////////////////////

// These will need to go under their respective objects
typedef uint32_t ApeWorldNodeMagic;

typedef enum ApeWorldNodeType
{
	APE_WORLD_NODE_TYPE_EMPTY = 0,
	APE_WORLD_NODE_TYPE_ROOT, // the world itself
	APE_WORLD_NODE_TYPE_ROOM, // and then brushes, lights, cameras and entities should be attached to this
	APE_WORLD_NODE_TYPE_BRUSH,// geometry that makes up the room
	APE_WORLD_NODE_TYPE_MODEL,
	APE_WORLD_NODE_TYPE_LIGHT,
	APE_WORLD_NODE_TYPE_CAMERA,
	APE_WORLD_NODE_TYPE_ENTITY,// can add dynamic behaviours to any children

	APE_WORLD_MAX_NODE_TYPES
} ApeWorldNodeType;

typedef struct ApeWorldNodeClass
{
	const char *identifier;
	ApeWorldNodeMagic magic;
	void ( *destroyFunction )( void *data );
} ApeWorldNodeClass;

typedef struct ApeWorldNode
{
	ApeWorldNodeMagic magic;

	char name[ 64 ];
	ApeWorldNodeType type;
	const ApeWorldNodeClass *classType;

	void *data;

	// Originally used a transform matrix here, but for simplicity...
	PLVector3 position;
	PLVector3 angles;
	PLVector3 scale;

	PLCollisionAABB localBounds;// bounds that aren't influenced by child, just whatever is specific to the node
	PLCollisionAABB bounds;     // bounds which resemble the local bounds of the node and all it's children

	ApeWorldNode *parent;
	struct PLLinkedListNode *parentListNode;// our slot under the parent

	struct PLLinkedList *children;// ApeWorldNode
} ApeWorldNode;

bool ape_world_node_is_valid_( const ApeWorldNode *self, ApeWorldNodeType expectedType );

ApeWorldNode *ape_world_node_setup_( ApeWorldNode *self, ApeWorldNode *parent, ApeWorldNodeType type, const PLVector3 *position, const PLVector3 *angles );
void ape_world_node_destroy( ApeWorldNode *self );

void ape_world_node_dettach( ApeWorldNode *self );
void ape_world_node_attach( ApeWorldNode *self, ApeWorldNode *parent );

PLVector3 ape_world_node_get_position( const ApeWorldNode *self );
void ape_world_node_set_position( ApeWorldNode *self, const PLVector3 *position );

PLVector3 ape_world_node_get_angles( const ApeWorldNode *self );
void ape_world_node_set_angles( ApeWorldNode *self, const PLVector3 *angles );

void ape_world_node_set_local_bounds( ApeWorldNode *self, const PLVector3 *mins, const PLVector3 *maxs );

/**
 * Travels up the tree until it encounters a room.
 */
ApeRoom *ape_world_node_get_room( ApeWorldNode *self );

ApeWorldNode *ape_world_node_get_root( ApeWorldNode *self );
ApeWorldNode *ape_world_node_get_child_by_name( ApeWorldNode *self, const char *name );

/////////////////////////////////////////////////////////////////////////////////////
// Ape World Brush - the building blocks of the world.
// Unlike most engines, you can define your own brush types to APE - so if you want
// to, say, introduce a brush that works like a minecraft cube, you can do so, or you
// can introduce a terrain brush type, or a mesh brush type, etc.
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeBrush ApeBrush;

typedef enum ApeBrushType
{
	APE_WORLD_BRUSH_TYPE_SOLID,
	APE_WORLD_BRUSH_TYPE_AIR,

	APE_MAX_WORLD_BRUSH_TYPES
} ApeBrushType;

#define APE_BRUSH_MAX_SUB_MESHES 8192

typedef struct ApeBrushFaceVertex
{
	PLVector2 textureCoords;
	PLVector2 lightmapCoords;
	PLVector3 position;
	PLVector3 normal;
	PLColourF32 colour;
} ApeBrushFaceVertex;

typedef struct ApeBrushFace
{
	int materialIndex;

	PLVectorArray *vertices;//ApeBrushFaceVertex
	PLLinkedList *edgeLoop; //ApeBrushFaceVertex

	unsigned int flags;

	struct ApeBrushFace *connectedPortalFace;
} ApeBrushFace;

typedef struct ApeBrush
{
	// This should always come first!
	ApeWorldNode base;

	ApeBrushType type;

	ApeMaterial *materials[ APE_BRUSH_MAX_SUB_MESHES ];

	PLVectorArray *faces;//ApeBrushFace

	PLGMesh *mesh;    // cached mesh
	bool isMeshCached;// if false, mesh cache will be updated
} ApeBrush;

ApeBrush *ape_create_brush( ApeWorldNode *parent, const PLVector3 *position, const PLVector3 *angles );

void ape_brush_destroy( ApeBrush *self );
void ape_brush_draw( ApeBrush *self );

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
	// This should always come first!
	ApeWorldNode base;

	char *name;
	PLPath path;

	PLVectorArray *meshes;

	PLLinkedList *entitySpawns;

	PLVectorArray *materials;// ApeMaterial
	PLVectorArray *rooms;    // ApeWorldRoom
	PLVectorArray *vertices; // ApeWorldVertex

	PLColourF32 ambience;

	PLColourF32 clearColour;

	PLColourF32 fogColour;
	float fogNear;
	float fogFar;

	/* additional generic properties */
	struct NdBranch *globalProperties;
} ApeWorld;

#define APE_WORLD_VERSION       3
#define APE_WORLD_EXTENSION     "wld.n"
#define APE_WORLD_EXTENSION_CFG "wpf.n"

/// Create an entirely new empty world handle.
/// \return New world instance.
ApeWorld *ape_create_world( void );

ApeWorld *ape_world_load( const char *path );

/// Attempts to save the given world to the destination.
/// \param self
/// \param path
/// \return On success, returns true but false otherwise.
bool ape_world_save( ApeWorld *self, const char *path );

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

/**
 * This crudely tries to determine the sector by an origin point.
 * Should only be used for vague, but fast, lookup.
 */
ApeRoom *ape_world_get_room_at_position( ApeWorld *world, const PLVector3 *position );

unsigned int ape_sky_add_layer( const char *path, float scale, float y, float alpha );
void ape_sky_set_layer_alpha( unsigned int slot, float alpha );
void ape_sky_set_layer_offset( unsigned int slot, float x, float y );
void ape_sky_clear_layers( void );
void ape_sky_draw_( ApeCamera *camera );

////////////////////////////////////////////////////////////////////
// Room

ApeRoom *ape_room_create( ApeWorldNode *parent );
void ape_world_room_destroy( ApeRoom *self );

////////////////////////////////////////////////////////////////////
// Face

void ape_world_face_generate_bounds( ApeWorldFace *face );

bool ape_world_face_is_mirror( const ApeWorldFace *self );
bool ape_world_face_is_portal( const ApeWorldFace *self );

////////////////////////////////////////////////////////////////////
// Lighting

typedef struct ApeMaterial ApeMaterial;

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

typedef enum ApeLightFlag
{
	PL_BITFLAG( APE_LIGHT_FLAG_DYNAMIC, 0U ),        // means the light is not baked, and can be moved at runtime
	PL_BITFLAG( SS_ARL_LIGHT_FLAG_SHADOWS, 1U ),        // if enabled without runtime shadows flag, will cast lightmap shadows
	PL_BITFLAG( APE_LIGHT_FLAG_RUNTIME_SHADOWS, 2U ),// treated as stencil shadow volumes
	PL_BITFLAG( APE_LIGHT_FLAG_ENABLED, 3U ),        // light will only be active if this flag is present
	PL_BITFLAG( APE_LIGHT_FLAG_FLARE, 4U ),             // light will produce a lensflare effect when visible
} ApeLightFlag;

/// A light can only be spawned in while the world is active.
/// \param type 	The type of light to be created.
/// \param position Position of the light.
/// \return 		A pointer to the instance of the light. This is owned by the world.
ApeLight *ape_create_light( ApeWorldNode *parent, const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, unsigned int flags );
void ape_light_destroy( ApeLight *light );

PLColourF32 ape_light_get_colour( const ApeLight *light );
void ape_light_set_colour( ApeLight *light, const PLColourF32 *colour );

PLVector3 ape_light_get_position( const ApeLight *light );
void ape_light_set_position( ApeLight *light, const PLVector3 *position );

PLVector3 ape_light_get_angles( const ApeLight *self );
void ape_light_set_angles( ApeLight *self, const PLVector3 *angles );

ApeLightShadowType ape_light_get_shadow_type( const ApeLight *light );

bool ape_light_is_active( const ApeLight *light );

bool ape_light_test_plane( const ApeLight *self, const PLCollisionPlane *plane );
bool ape_light_test_plane_shadow( const ApeLight *self, const ApeMaterial *material, const PLCollisionPlane *plane );

PL_EXTERN_C_END
