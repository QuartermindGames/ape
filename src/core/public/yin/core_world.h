// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_physics.h>

#include "ape/ape_public_audio.h"

PL_EXTERN_C

typedef struct PLVectorArray PLVectorArray;
typedef struct PLLinkedList  PLLinkedList;

typedef struct AcmBranch AcmBranch;

/* external elements */
typedef struct ApeMaterial ApeMaterial;
typedef struct ApeCamera   ApeCamera;
typedef struct ApeViewport ApeViewport;
typedef struct ApeLight    ApeLight;
typedef struct ApeEntity   ApeEntity;// core_entity.h
typedef struct ApeRoom     ApeRoom;  // world.h
typedef struct ApeBrush    ApeBrush;
typedef struct ApeWorld    ApeWorld;

/* ======================================================================
 * WORLD INTERFACE
 * ====================================================================*/

typedef struct ApeWorldNode ApeWorldNode;
#define APE_WORLD_NODE( X ) ( ( ApeWorldNode * ) ( X ) )

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
	const char       *identifier;
	ApeWorldNodeMagic magic;
	void ( *destroyFunction )( void *data );
	AcmBranch *( *serializeFunction )( void *data, AcmBranch *root );
} ApeWorldNodeClass;

typedef struct ApeWorldNode
{
	ApeWorldNodeMagic magic;

	char                     name[ 64 ];
	ApeWorldNodeType         type;
	const ApeWorldNodeClass *classType;

	//todo: remove these once transform matrix is used more widely
	PLVector3 position;
	PLVector3 angles;
	PLVector3 scale;

	PLMatrix4 transform;

	PLCollisionAABB localBounds;// bounds that aren't influenced by child, just whatever is specific to the node
	PLCollisionAABB bounds;     // bounds which resemble the local bounds of the node and all it's children

	ApeWorldNode            *parent;
	struct PLLinkedListNode *parentListNode;// our slot under the parent

#if !defined( APE_NO_EDITOR )
	PLColour selectColour;
#endif

	struct PLLinkedList *children;// ApeWorldNode
} ApeWorldNode;

bool ape_world_node_is_valid( const ApeWorldNode *self, ApeWorldNodeType expectedType );

ApeWorldNode *ape_world_node_setup_( ApeWorldNode *self, ApeWorldNode *parent, ApeWorldNodeType type, const char *name, const PLVector3 *position, const PLVector3 *angles );
void          ape_world_node_destroy( ApeWorldNode *self );

void ape_world_node_dettach( ApeWorldNode *self );
void ape_world_node_attach( ApeWorldNode *self, ApeWorldNode *parent );

PLVector3 ape_world_node_get_position( const ApeWorldNode *self );
void      ape_world_node_set_position( ApeWorldNode *self, const PLVector3 *position );

PLVector3 ape_world_node_get_angles( const ApeWorldNode *self );
void      ape_world_node_set_angles( ApeWorldNode *self, const PLVector3 *angles );

void ape_world_node_set_local_bounds( ApeWorldNode *self, const PLVector3 *mins, const PLVector3 *maxs );

/**
 * Travels up the tree until it encounters a room.
 */
ApeRoom *ape_world_node_get_room( ApeWorldNode *self );

/**
 * Attaches the given node, and in-turn its children, to the specific room.
 *
 * @param self	Instance of the node.
 * @param room 	Pointer to the specific room.
 */
void ape_world_node_set_room( ApeWorldNode *self, ApeRoom *room );

ApeWorldNode *ape_world_node_get_root( ApeWorldNode *self );
ApeWorldNode *ape_world_node_get_child_by_name( ApeWorldNode *self, const char *name );

/**
 * Fetch the name of the given node.
 *
 * @param self 	Instance of the node.
 * @return 		Name of the node.
 */
const char *ape_world_node_get_name( ApeWorldNode *self );

/**
 * Set the name of the given node.
 *
 * @param self 	Instance of the node.
 * @param name 	New name to set.
 */
void ape_world_node_set_name( ApeWorldNode *self, const char *name );

AcmBranch *ape_world_node_serialize( ApeWorldNode *self, AcmBranch *root );

#define APE_SG_NODE_GET_POSITION( X ) ape_world_node_get_position( ( ApeWorldNode * ) ( X ) )
#define APE_SG_NODE_DESTROY( X )      ape_world_node_destroy( ( ApeWorldNode * ) ( X ) )

/////////////////////////////////////////////////////////////////////////////////////
// Ape World Brush - the building blocks of the world.
// Unlike most engines, you can define your own brush types to APE - so if you want
// to, say, introduce a brush that works like a minecraft cube, you can do so, or you
// can introduce a terrain brush type, or a mesh brush type, etc.
/////////////////////////////////////////////////////////////////////////////////////

#define APE_BRUSH_MAX_FACE_VERTICES 16

typedef struct ApeBrush     ApeBrush;
typedef struct ApeBrushFace ApeBrushFace;

typedef enum ApeBrushType
{
	APE_WORLD_BRUSH_TYPE_SOLID,
	APE_WORLD_BRUSH_TYPE_AIR,

	APE_MAX_WORLD_BRUSH_TYPES
} ApeBrushType;

typedef enum ApeBrushFaceFlag
{
	PL_BITFLAG( APE_BRUSH_FACE_FLAG_HIDDEN, 0 ),
	PL_BITFLAG( APE_BRUSH_FACE_FLAG_MIRROR, 1 ),
} ApeBrushFaceFlag;

#define APE_BRUSH_MAX_SUB_MESHES 8192

typedef struct ApeBrushFaceVertex
{
	PLVector3  *position;
	PLVector2   textureCoords;
	PLVector3   tangent, bitangent;
	PLVector2   lightmapCoords;
	PLVector3   normal;
	PLColourF32 colour;
} ApeBrushFaceVertex;

typedef struct ApeBrushFace
{
	int          materialIndex;
	ApeMaterial *material;
	PLVector2    materialScale;
	PLVector3    materialOffset;
	PLVector3    materialAngle;

	PLVector3   normal;
	PLColourF32 colour;
	PLColour    selectColour;

	ApeBrushFaceVertex *edgeLoop[ APE_BRUSH_MAX_FACE_VERTICES ];// represents the actual draw order
	ApeBrushFaceVertex  vertices[ APE_BRUSH_MAX_FACE_VERTICES ];// list of vertices
	uint                numVertices;

	PLCollisionAABB bounds;

	uint flags;

	char          id[ 64 ];// required for connecting portals
	ApeBrushFace *destination;
	ApeBrush     *parent;
} ApeBrushFace;

typedef struct ApeBrush
{
	// This should always come first!
	ApeWorldNode base;

	ApeBrushType type;

	ApeMaterial *materials[ APE_BRUSH_MAX_SUB_MESHES ];

	PLVector3 *vertices;
	uint       numVertices;

	ApeBrushFace *faces;
	uint          numFaces;
} ApeBrush;

ApeBrush *ape_brush_create( ApeWorldNode *parent, const char *name, const PLVector3 *position, const PLVector3 *angles );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

#define APE_USE_NEW_WORLD_LAYOUT

typedef struct ApeWorldFace   ApeWorldFace;
typedef struct ApeWorldMesh   ApeWorldMesh;
typedef struct ApeWorldObject ApeWorldObject;
typedef struct ApeRoom        ApeRoom;

typedef struct PLVectorArray PLVectorArray;
typedef struct PLLinkedList  PLLinkedList;

typedef struct ApeWorld
{
	// This should always come first!
	ApeWorldNode base;

	char  *name;
	PLPath path;

	PLVectorArray *meshes;

	PLLinkedList *entitySpawns;

	PLVectorArray *materials;// ApeMaterial
	PLVectorArray *rooms;    // ApeWorldRoom
	PLVectorArray *vertices; // ApeWorldVertex

	PLColourF32 ambience;

	PLColourF32 clearColour;

	PLColourF32 fogColour;
	float       fogNear;
	float       fogFar;

	/* additional generic properties */
	struct AcmBranch *globalProperties;
} ApeWorld;

#define APE_WORLD_VERSION       3
#define APE_WORLD_EXTENSION     "wld.n"
#define APE_WORLD_EXTENSION_CFG "wpf.n"

#define APE_WORLD_ROOM_VERSION   1
#define APE_WORLD_ROOM_EXTENSION "rom.n"

#define APE_WORLD_BRUSH_VERSION   1
#define APE_WORLD_BRUSH_EXTENSION "brs.n"

/// Create an entirely new empty world handle.
/// \return New world instance.
ApeWorld *ape_create_world( void );

ApeWorld *ape_world_load( const char *path );

/// Attempts to save the given world to the destination.
/// \param self
/// \param path
/// \return On success, returns true but false otherwise.
bool ape_world_save( ApeWorld *self, const char *path );

AcmBranch *apeGetWorldProperty( ApeWorld *world, const char *propertyName );

// TODO: move these under the renderer sub-system
void ape_world_draw_wireframe( ApeWorld *world, ApeCamera *camera );
void ape_world_draw( ApeWorld *world, ApeCamera *camera, ApeLight *light, bool ambienceOnly, bool alpha );
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

uint ape_sky_add_layer( const char *path, float scale, float y, float alpha );
void ape_sky_set_layer_alpha( uint slot, float alpha );
void ape_sky_set_layer_offset( uint slot, float x, float y );
void ape_sky_clear_layers( void );
void ape_sky_draw_( ApeCamera *camera );

////////////////////////////////////////////////////////////////////
// Room

/**
 * Creates a new ApeRoom object.
 *
 * This function allocates memory for a new ApeRoom and initializes it with
 * the provided parent and name. It assigns default values to the room's
 * fields, including a random color for debugging purposes.
 *
 * @param parent 	Pointer to the parent ApeWorldNode. The newly created
 *        			ApeRoom will be set up as a child of this node.
 * @param name 		A string representing the name of the new ApeRoom. This name
 *        			will be used to identify the room.
 * @return 			A pointer to the newly created ApeRoom.
 */
ApeRoom *ape_room_create( ApeWorldNode *parent, const char *name );

void        ape_room_set_ambience( ApeRoom *self, PLColourF32 ambience );
PLColourF32 ape_room_get_ambience( ApeRoom *self );

void                 ape_room_set_reverb_preset( ApeRoom *self, ApeAudioReverbPreset reverbPreset );
ApeAudioReverbPreset ape_room_get_reverb_preset( ApeRoom *self );

bool        ape_room_set_path( ApeRoom *self, const char *path );
const char *ape_room_get_path( const ApeRoom *self );

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
	APE_LIGHT_SHADOW_TYPE_NONE,
	APE_LIGHT_SHADOW_TYPE_DYNAMIC,
	APE_LIGHT_SHADOW_TYPE_STATIC,

	APE_MAX_LIGHT_SHADOW_TYPES
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
	PL_BITFLAG( SS_ARL_LIGHT_FLAG_SHADOWS, 1U ),     // if enabled without runtime shadows flag, will cast lightmap shadows
	PL_BITFLAG( APE_LIGHT_FLAG_RUNTIME_SHADOWS, 2U ),// treated as stencil shadow volumes
	PL_BITFLAG( APE_LIGHT_FLAG_ENABLED, 3U ),        // light will only be active if this flag is present
	PL_BITFLAG( APE_LIGHT_FLAG_FLARE, 4U ),          // light will produce a lensflare effect when visible
} ApeLightFlag;

/// A light can only be spawned in while the world is active.
/// \param type 	The type of light to be created.
/// \param position Position of the light.
/// \return 		A pointer to the instance of the light. This is owned by the world.
ApeLight *ape_create_light( ApeWorldNode *parent, const PLVector3 *position, const PLColourF32 *colour, float radius, ApeLightType type, uint flags );
void      ape_light_destroy( ApeLight *light );

PLColourF32 ape_light_get_colour( const ApeLight *light );
void        ape_light_set_colour( ApeLight *light, const PLColourF32 *colour );

PLVector3 ape_light_get_position( const ApeLight *light );
void      ape_light_set_position( ApeLight *light, const PLVector3 *position );

PLVector3 ape_light_get_angles( const ApeLight *self );
void      ape_light_set_angles( ApeLight *self, const PLVector3 *angles );

void ape_light_set_radius( ApeLight *self, float radius );

ApeLightShadowType ape_light_get_shadow_type( const ApeLight *light );

bool ape_light_is_active( const ApeLight *light );

bool ape_light_test_plane( const ApeLight *self, const PLCollisionPlane *plane );
bool ape_light_test_plane_shadow( const ApeLight *self, const ApeMaterial *material, const PLCollisionPlane *plane );

/**
 * Test whether or not the light is visible to the given camera.
 *
 * @param self 		Instance of light.
 * @param camera 	Instance of camera.
 * @return 			True on visible, false otherwise.
 */
bool ape_light_is_visible( const ApeLight *self, const ApeCamera *camera );

PL_EXTERN_C_END
