// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_physics.h>

//TODO: ?????
#include "ape_public_audio.h"

PL_EXTERN_C

typedef struct QmOsLinkedList QmOsLinkedList;
typedef struct QmOsSharedPtr  QmOsSharedPtr;

typedef struct AcmBranch AcmBranch;

typedef struct ApeMaterial ApeMaterial;
typedef struct ApeLight    ApeLight;
typedef struct ApeEntity   ApeEntity;// core_entity.h
typedef struct ApeRoom     ApeRoom;  // world.h
typedef struct ApeBrush    ApeBrush;

typedef struct ApeProperty ApeProperty;

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

typedef enum ApeWorldNodeClassFlag
{
	QM_OS_BIT_FLAG( APE_WORLD_NODE_CLASS_FLAG_NO_EDITOR, 0 ),
} ApeWorldNodeClassFlag;

typedef void *( *ApeWorldNodeClassNetSerializeFunction )( ApeWorldNode *self, unsigned int *dstLength );
typedef void ( *ApeWorldNodeClassNetDeserializeFunction )( ApeWorldNode *self, void *newState, unsigned int length );

//TODO: why is this public!?
typedef struct ApeWorldNodeClass
{
	const char       *identifier;
	ApeWorldNodeMagic magic;

	void *( *create )( ApeWorldNode *parent );
	void ( *destroy )( void *self, ApeWorldNode *parent );
	AcmBranch *( *serialize )( void *self, AcmBranch *root );
	ApeWorldNode *( *deserialize )( ApeWorldNode *self, AcmBranch *root );
	ApeWorldNode *( *clone )( ApeWorldNode *srcNode );

	void ( *onAttachChild )( void *self, ApeWorldNode *child ); // called just after a child is attached
	void ( *onDettachChild )( void *self, ApeWorldNode *child );// called just before a child is dettached

	void ( *onAttachParent )( void *self, ApeWorldNode *parent ); // called just after a parent is attached
	void ( *onDettachParent )( void *self, ApeWorldNode *parent );// called just after a parent is dettached

	void ( *onChangeRoom )( void *self, ApeRoom *currentRoom, ApeRoom *newRoom );// called when the node moves from one room to another, just before the change is set

	ApeWorldNodeClassNetSerializeFunction   netSerializeFunction;
	ApeWorldNodeClassNetDeserializeFunction netDeserializeFunction;

	const ApeProperty *properties;
	unsigned int       numProperties;

#if defined( APE_SUPPORT_EDITOR )
	const char *editorIcon;

	/**
	 * This gets called regardless of being selected, or not.
	 * However, in some cases you might only want your drawn
	 * elements to show in the situation that it's selected,
	 * which is what the 'isSelected' flag is for.
	 */
	void ( *onDrawEditor )( void *self, bool isSelected );
#endif

	unsigned int flags;
} ApeWorldNodeClass;

typedef enum ApeWorldNodeFlag
{
	QM_OS_BIT_FLAG( APE_WORLD_NODE_FLAG_HIDDEN, 0 ), // shouldn't be displayed visually
	QM_OS_BIT_FLAG( APE_WORLD_NODE_FLAG_DISCARD, 1 ),// if marked, won't be serialized and will be discarded
} ApeWorldNodeFlag;

//TODO: why is this public!?
typedef struct ApeWorldNode
{
	ApeWorldNodeMagic magic;

	char                     name[ 64 ];
	ApeWorldNodeType         type;
	const ApeWorldNodeClass *classType;

	//todo: remove these once transform matrix is used more widely
	QmMathVector3f position;
	QmMathVector3f angles;
	QmMathVector3f scale;

	PLMatrix4 localTransform;
	PLMatrix4 worldTransform;

	PLCollisionAABB localBounds;// bounds that aren't influenced by child, just whatever is specific to the node
	PLCollisionAABB bounds;     // bounds which resemble the local bounds of the node and all it's children

	bool needsSyncOnConnect;
	bool needsSyncOnTick;

	struct PLGMesh *mesh;       // used for brush geometry childed to the given node
	bool            isMeshDirty;// indicates the mesh needs updating

	ApeRoom                 *room;
	ApeWorldNode            *parent;
	struct PLLinkedListNode *parentListNode;// our slot under the parent

#if defined( APE_SUPPORT_EDITOR )
	QmMathColour4ub selectColour;
#endif

	unsigned int flags;

	PLPath path;// where we were loaded from, if at all (note this isn't always valid)
#if defined( APE_SUPPORT_EDITOR )
	PLPath savePath;
#endif

	struct PLLinkedList *children;// ApeWorldNode
} ApeWorldNode;

/**
 * Returns a list of all available node classes.
 *
 * @param numClasses	Total number of classes available.
 * @return				Pointer to the list of available classes.
 */
const ApeWorldNodeClass **ape_world_node_get_classes( unsigned int *numClasses );

/**
 * Returns the global list of properties for a world node.
 *
 * @param numProperties	Total number of properties available.
 * @return				Pointer to the list of properties.
 */
const ApeProperty *ape_world_node_get_properties( unsigned int *numProperties );

/**
 * Returns the list of properties for the given node type.
 *
 * @param numProperties Total number of properties available.
 * @param type			World node class type.
 * @return				Pointer to the list of properties.
 */
const ApeProperty *ape_world_node_get_class_properties( unsigned int *numProperties, ApeWorldNodeType type );

void *ape_world_node_get_property_pointer( ApeWorldNode *self, const ApeProperty *property );

bool ape_world_node_has_magic( const ApeWorldNode *self );
bool ape_world_node_is_valid( const ApeWorldNode *self, ApeWorldNodeType expectedType );

void ape_world_node_destroy( ApeWorldNode *self );

void ape_world_node_dettach( ApeWorldNode *self );
void ape_world_node_attach( ApeWorldNode *self, ApeWorldNode *parent );

QmMathVector3f ape_world_node_get_local_position( const ApeWorldNode *self );
QmMathVector3f ape_world_node_get_position( const ApeWorldNode *self );
void           ape_world_node_set_position( ApeWorldNode *self, const QmMathVector3f *position );

/* X = pitch
 * Y = yaw
 * Z = roll
 */

QmMathVector3f ape_world_node_get_angles( const ApeWorldNode *self );
void           ape_world_node_set_angles( ApeWorldNode *self, const QmMathVector3f *angles );

void ape_world_node_set_local_bounds( ApeWorldNode *self, const QmMathVector3f *mins, const QmMathVector3f *maxs );

ApeWorldNode *ape_world_node_get_parent_by_type( const ApeWorldNode *self, ApeWorldNodeType type );
ApeWorldNode *ape_world_node_get_parent_by_name( const ApeWorldNode *self, const char *name );

bool ape_world_node_is_descendant_of_node( const ApeWorldNode *self, const ApeWorldNode *lookup );

/**
 * Returns the room the given node is attached to.
 * @param self	Instance of the node.
 * @return		Pointer to the room the node is attached to.
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
ApeWorldNode *ape_world_node_get_child_by_name( const ApeWorldNode *self, const char *name );
ApeWorldNode *ape_world_node_get_descendant_by_name( const ApeWorldNode *self, const char *name );

/**
 * Fetch the name of the given node.
 *
 * @param self 	Instance of the node.
 * @return 		Name of the node.
 */
const char *ape_world_node_get_name( const ApeWorldNode *self );

/**
 * Set the name of the given node.
 *
 * @param self 	Instance of the node.
 * @param name 	New name to set.
 */
void ape_world_node_set_name( ApeWorldNode *self, const char *name );

PLCollisionAABB ape_world_node_get_transformed_local_bounds( const ApeWorldNode *self );
PLCollisionAABB ape_world_node_get_local_bounds( const ApeWorldNode *self );
PLCollisionAABB ape_world_node_get_bounds( const ApeWorldNode *self );

/**
 * Fetch the world transform.
 *
 * @param self	Instance of the node.
 * @return		World transform.
 */
PLMatrix4 ape_world_node_get_transform( const ApeWorldNode *self );

/**
 * Fetch the local transform.
 *
 * @param self Instance of the node.
 * @return Local transform.
 */
PLMatrix4 ape_world_node_get_local_transform( const ApeWorldNode *self );

/**
 * Returns the forward vector relative to the local angles.
 * Mind this isn't cached, so use sparingly for now.
 * @param self Instance of the node.
 * @return Forward vector.
 */
QmMathVector3f ape_world_node_get_forward( const ApeWorldNode *self );

AcmBranch *ape_world_node_serialize( ApeWorldNode *self, AcmBranch *root );

/**
 * Iterate through all of the children of the given node and collect up children by type.
 * If you just want to iterate over the top-level, you're probably better off just addressing directly rather than using this method!
 *
 * @param self			Instance of the node.
 * @param type			Type of world node to gather.
 * @param numChildren	Number of children returned.
 * @param recursive		Whether to recurse through all child nodes.
 * @return				Array of all children nodes found.
 */
ApeWorldNode **ape_world_node_gather_children( ApeWorldNode *self, ApeWorldNodeType type, unsigned int *numChildren, bool recursive );

/**
 * Iterates through all of the children of the given node and executes a callback when the specified type is encountered.
 *
 * @param self		Instance of the node.
 * @param type		Type of world node to visit.
 * @param recursive	Whether to recurse through all child nodes.
 * @param callback	Callback method to call if specified type encountered.
 * @param user		User pointer to pass into callback.
 * @return			Number of children visited.
 */
unsigned int ape_world_node_visit_children( ApeWorldNode *self, ApeWorldNodeType type, bool recursive, bool ( *callback )( ApeWorldNode *self, void *user ), void *user );

/**
 * Attempt to load a world node from a given path.
 *
 * @param parent	Parent to attach to.
 * @param path		Path to load the world node from.
 * @return			Instance of the new world node on success, null on failure.
 */
ApeWorldNode *ape_world_node_load( ApeWorldNode *parent, const char *path );

/**
 * Fetch the path that the node was originally loaded from.
 *
 * @param self	Instance of the node.
 * @return		Pointer to string; it will be empty if the node wasn't loaded from disk
 */
const char *ape_world_node_get_path( const ApeWorldNode *self );

ApeWorldNode *ape_world_node_get_parent( ApeWorldNode *self );

/////////////////////////////////////////////////////////////////////////////////////
// Brush - the building blocks of the world.
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
	QM_OS_BIT_FLAG( APE_BRUSH_FACE_FLAG_HIDDEN, 0U ),// hides the face
	QM_OS_BIT_FLAG( APE_BRUSH_FACE_FLAG_MIRROR, 1U ),// reflects the current room
	QM_OS_BIT_FLAG( APE_BRUSH_FACE_FLAG_PORTAL, 2U ),// allows for passing through to another destination
	QM_OS_BIT_FLAG( APE_BRUSH_FACE_FLAG_CLOSED, 3U ),// if a portal/mirror, indicates that the vis-test shouldn't pass
} ApeBrushFaceFlag;

typedef struct ApeBrushFaceVertex
{
	unsigned int   posIndex;
	QmMathVector2f textureCoords;
	QmMathVector2f lightmapCoords;
	QmMathVector3f normal;
	QmMathColour4f colour;
} ApeBrushFaceVertex;

////////////////////////////////////////////////////////////////////
// Brush Face

static constexpr unsigned int APE_BRUSH_FACE_MAX_TAG  = 64;
static constexpr unsigned int APE_BRUSH_FACE_MAX_PATH = sizeof( PLPath );

static constexpr uint8_t APE_BRUSH_FACE_LIGHTMAP_INVALID        = ( uint8_t ) -1;
static constexpr uint8_t APE_BRUSH_FACE_LIGHTMAP_DEFAULT_LUXELS = 4;

typedef struct ApeBrushFace
{
	ApeMaterial   *material;
	QmMathVector2f materialScale;
	QmMathVector3f materialOffset;
	QmMathVector3f materialAngle;

	QmMathVector3f tangent;
	QmMathVector3f bitangent;

	QmMathVector3f  normal;
	QmMathColour4f  colour;
	QmMathColour4ub selectColour;

	unsigned int       edgeLoopOrder[ APE_BRUSH_MAX_FACE_VERTICES ];// represents the actual draw order
	ApeBrushFaceVertex vertices[ APE_BRUSH_MAX_FACE_VERTICES ];     // list of vertices
	unsigned int       numVertices;

	PLCollisionAABB bounds;//TODO: replace this with a sphere check

	QmMathVector4f lightmapArea;
	uint8_t        lightmapIndex;
	uint8_t        lightmapLuxelDensity;

	unsigned int flags;

	char          tag[ APE_BRUSH_FACE_MAX_TAG ];                                     // how we can be found
	char          destinationTag[ APE_BRUSH_FACE_MAX_PATH + APE_BRUSH_FACE_MAX_TAG ];// where we're going
	ApeBrushFace *destination;

	ApeBrush *parent;

	struct QmOsSharedPtr *ptr;
} ApeBrushFace;

void ape_brush_face_setup( ApeBrushFace *self );

void ape_brush_face_fit_material( ApeBrushFace *self );

void ape_brush_face_apply_material( ApeBrushFace *self, ApeMaterial *material );
void ape_brush_face_apply_material_coordinates( ApeBrushFace *self, const QmMathVector2f *scale, const QmMathVector2f *offset, const QmMathVector3f *rotation, bool computeLocal );

bool           ape_brush_face_is_emissive( const ApeBrushFace *self );
QmMathColour4f ape_brush_face_get_emission( const ApeBrushFace *self );

bool          ape_brush_face_is_portal( const ApeBrushFace *self );
bool          ape_brush_face_is_mirror( const ApeBrushFace *self );
ApeBrushFace *ape_brush_face_get_portal_destination( ApeBrushFace *self );

ApeRoom *ape_brush_face_get_room( const ApeBrushFace *self );

bool ape_brush_face_set_tag( ApeBrushFace *self, const char *tag );

void ape_brush_face_compute_normal( ApeBrushFace *face );
void ape_brush_face_compute_bounds( ApeBrushFace *face );

////////////////////////////////////////////////////////////////////
// Brush

typedef enum ApeBrushLightingType : uint8_t
{
	APE_BRUSH_LIGHTING_TYPE_LIGHTMAP,
	APE_BRUSH_LIGHTING_TYPE_VERTEX,
} ApeBrushLightingType;

typedef struct ApeBrush
{
	// This should always come first!
	ApeWorldNode base;

	ApeBrushType         type;
	ApeBrushLightingType lightingType;

	QmMathVector3f *vertices;
	unsigned int    numVertices;

#if defined( APE_SUPPORT_EDITOR )
	QmMathColour4ub *vertexSelectColours;   // colours used for selection of vertices
	unsigned int     numVertexSelectColours;// should match num vertices
#endif

	ApeBrushFace *faces;
	unsigned int  numFaces;
} ApeBrush;

ApeBrush *ape_brush_create( ApeWorldNode *parent, const char *name, const QmMathVector3f *position, const QmMathVector3f *angles );

void ape_brush_compute_bounds( ApeBrush *self );
void ape_brush_compute_face_bounds( ApeBrush *self );
void ape_brush_compute_face_normals( ApeBrush *self );

void ape_brush_mark_parent_dirty( ApeBrush *self );

void ape_brush_merge_brushes( ApeBrush *self, ApeBrush **brushes, unsigned int numBrushes );

/**
 * Iterates over all the faces in the given list,
 * and ensures smooth normals across them all.
 * @param faces Linked list of faces to smooth.
 */
void ape_brush_smooth_faces( const QmOsLinkedList *faces );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

#define APE_USE_NEW_WORLD_LAYOUT

typedef struct ApeRoom ApeRoom;

typedef struct PLVectorArray PLVectorArray;
typedef struct PLLinkedList  PLLinkedList;

typedef struct ApeWorld
{
	// This should always come first!
	ApeWorldNode base;

	char  *name;
	PLPath path;

	PLVectorArray *materials;// ApeMaterial

	PLLinkedList       *entities;//ApeEntity
	struct PLHashTable *roomLookup;
} ApeWorld;

#define APE_WORLD_VERSION   3
#define APE_WORLD_EXTENSION "wld.n"

#define APE_WORLD_ROOM_VERSION   1
#define APE_WORLD_ROOM_EXTENSION "rom.n"

#define APE_WORLD_BRUSH_EXTENSION "brs.n"

/// Create an entirely new empty world handle.
/// \return New world instance.
ApeWorld *ape_world_create( void );

void ape_world_spawn_entities( ApeWorld *self );

void ape_world_compute_light_grids( ApeWorld *self );

ApeRoom *ape_world_get_room_by_path( ApeWorld *self, const char *path );

/**
 * Lookup a tagged surface based on the given path.
 *
 * @param self	World instance.
 * @param path	Path to lookup the tagged surface ('rooms/myroom.rom.n:surfacename').
 * @return		Surface/face, otherwise null on fail.
 */
ApeBrushFace *ape_world_get_tagged_surface( ApeWorld *self, const char *path );

/**
 * Fetch all the available tagged surfaces.
 *
 * @param self		World instance.
 * @param numDst	Number of tagged surfaces returned.
 * @return			An allocated array of tagged surfaces; free after use!
 */
ApeBrushFace **ape_world_get_tagged_surfaces( ApeWorld *self, unsigned int *numDst );

/**
 * Assigning a light to the world will give the world instance
 * ownership of that light.
 */
void ape_world_attach_light( ApeWorld *world, ApeLight *light );

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

void           ape_room_set_ambience( ApeRoom *self, QmMathColour4f ambience );
QmMathColour4f ape_room_get_ambience( const ApeRoom *self );

void                 ape_room_set_reverb_preset( ApeRoom *self, ApeAudioReverbPreset reverbPreset );
ApeAudioReverbPreset ape_room_get_reverb_preset( const ApeRoom *self );

bool ape_room_get_light_sample( ApeRoom *self, QmMathVector3f position, QmMathColour3f16 *dstLight, QmMathVector3f *dstDir );

void         ape_room_set_lightmap_edge_length( ApeRoom *self, unsigned int edgeLength );
unsigned int ape_room_get_lightmap_edge_length( const ApeRoom *self );

void ape_room_add_tagged_surface( ApeRoom *self, ApeBrushFace *face );
void ape_room_remove_tagged_surface( ApeRoom *self, ApeBrushFace *face );

ApeBrushFace *ape_room_get_tagged_surface( const ApeRoom *self, const char *tag );

const char *ape_room_set_unique_surface_tag( const ApeRoom *self, ApeBrushFace *face );

////////////////////////////////////////////////////////////////////
// Collisions

typedef struct ComCollisionCylinder ComCollisionCylinder;

typedef enum ApeCollisionType
{
	APE_COLLISION_TYPE_NONE     = 0,
	APE_COLLISION_TYPE_AABB     = 1,
	APE_COLLISION_TYPE_SPHERE   = 2,
	APE_COLLISION_TYPE_PLANE    = 3,
	APE_COLLISION_TYPE_CYLINDER = 4,
} ApeCollisionType;

typedef enum ApeCollisionGroup
{
	QM_OS_BIT_FLAG( APE_COLLISION_GROUP_BRUSHES, 0U ),
	QM_OS_BIT_FLAG( APE_COLLISION_GROUP_ENTITIES, 1U ),

	// games can provide custom flags after this...
	APE_COLLISION_GROUP_END = APE_COLLISION_GROUP_BRUSHES,
} ApeCollisionGroup;

typedef struct ApeCollisionCollider
{
	ApeCollisionType type;
	union
	{
		void                 *ptr;
		PLCollisionAABB      *aabb;
		PLCollisionSphere    *sphere;
		PLCollisionPlane     *plane;
		ComCollisionCylinder *cylinder;
	};

	ApeWorldNode **ignores;
	unsigned int   numIgnores;

	unsigned int ignoreGroups;
} ApeCollisionCollider;

typedef struct ApeCollisionIntersection
{
	ApeWorldNode  *node;        // the node that we hit
	ApeBrushFace  *face;        // face we hit, if any
	QmMathVector3f origin;      // origin position of the original collider
	QmMathVector3f intersection;// point on the node that we hit
	float          distance;    // distance from point of intersection vs. caster
	float          depth;
} ApeCollisionIntersection;

ApeCollisionIntersection *ape_room_intersect( ApeRoom *self, const ApeCollisionCollider *collider, unsigned int *numHits );

//TODO: obsolete
bool ape_room_ray_intersect( ApeRoom *self, const PLCollisionRay *ray, ApeCollisionIntersection *result );

QmMathVector3f ape_room_get_gravity( const ApeRoom *self );

////////////////////////////////////////////////////////////////////
// Decals

typedef struct ApeDecalManager ApeDecalManager;

/**
 * Attempts to trace a decal in the direction specified.
 */
bool ape_room_trace_decal( ApeRoom *self, ApeMaterial *material, const QmMathVector3f *pos, const QmMathVector3f *dir, float angle, float scale );

/**
 * Attempts to create a decal at the given position, for the given face.
 */
QmOsSharedPtr *ape_room_create_decal( const ApeRoom *self, ApeMaterial *material, ApeBrushFace *face, QmMathVector3f pos, float angle, float scale, bool isStatic );

/**
 * Fetch the decal manager from the given room.
 * Mind this will *eventually* get deprecated, as the manager is moved into the world!
 */
ApeDecalManager *ape_room_get_decal_manager( const ApeRoom *self );

////////////////////////////////////////////////////////////////////
// Lighting

typedef struct ApeMaterial ApeMaterial;

//#define APE_ENABLE_LIGHT_INV_SQUARE_FALLOFF

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
	QM_OS_BIT_FLAG( APE_LIGHT_FLAG_DYNAMIC, 0U ),        // means the light is not baked, and can be moved at runtime
	QM_OS_BIT_FLAG( APE_LIGHT_FLAG_SHADOWS, 1U ),        // if enabled without runtime shadows flag, will cast lightmap shadows
	QM_OS_BIT_FLAG( APE_LIGHT_FLAG_RUNTIME_SHADOWS, 2U ),// treated as stencil shadow volumes
	QM_OS_BIT_FLAG( APE_LIGHT_FLAG_ENABLED, 3U ),        // light will only be active if this flag is present
	QM_OS_BIT_FLAG( APE_LIGHT_FLAG_FLARE, 4U ),          // light will produce a lensflare effect when visible
} ApeLightFlag;

/// A light can only be spawned in while the world is active.
/// \param type 	The type of light to be created.
/// \param position Position of the light.
/// \return 		A pointer to the instance of the light. This is owned by the world.
ApeLight *ape_create_light( ApeWorldNode *parent, const QmMathVector3f *position, const QmMathColour4f *colour, float radius, ApeLightType type, unsigned int flags );
void      ape_light_destroy( ApeLight *light );

QmMathVector3f ape_light_get_direction( const ApeLight *self );

QmMathColour4f ape_light_get_colour( const ApeLight *self );
void           ape_light_set_colour( ApeLight *light, const QmMathColour4f *colour );

QmMathVector3f ape_light_get_position( const ApeLight *self );
void           ape_light_set_position( ApeLight *light, const QmMathVector3f *position );

QmMathVector3f ape_light_get_angles( const ApeLight *self );
void           ape_light_set_angles( ApeLight *self, const QmMathVector3f *angles );

ApeLightType ape_light_get_type( const ApeLight *self );
void         ape_light_set_type( ApeLight *self, ApeLightType type );

void ape_light_set_radius( ApeLight *self, float radius );

ApeLightShadowType ape_light_get_shadow_type( const ApeLight *light );

bool ape_light_is_active( const ApeLight *light );

bool ape_light_test_plane( const ApeLight *self, const PLCollisionPlane *plane );
bool ape_light_test_sphere( const ApeLight *self, const PLCollisionSphere *sphere );
bool ape_light_test_bounds( const ApeLight *self, QmMathVector3f origin, QmMathVector3f mins, QmMathVector3f maxs );
bool ape_light_test_face( const ApeLight *self, const ApeBrushFace *face );
bool ape_light_test_plane_shadow( const ApeLight *self, const ApeMaterial *material, const PLCollisionPlane *plane );

PL_EXTERN_C_END
