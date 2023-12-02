// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_array_vector.h>

#include <plgraphics/plg_mesh.h>

#include <yin/core_world.h>

#include "ape_memory_manager.h"
#include "entity/entity.h"
#include "client/audio/audio.h"

#define WORLD_PROP_TAG_LENGTH   64
#define WORLD_PROP_VALUE_LENGTH 256

#if 1 /* original values, used for prototype */
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.25f, 0.25f, 0.25f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.1f, 0.5f, 1.0f, 1.0f )
#else
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#endif

typedef struct PLFile PLFile;

typedef struct SSAclWorldRoom SSAclWorldRoom;
typedef struct SSAclWorldFaceVertex SSAclWorldFaceVertex;
typedef struct SSAclWorldFace SSAclWorldFace;
typedef struct SSAclWorldMesh SSAclWorldMesh;
typedef struct SSAclWorldPortal SSAclWorldPortal;

typedef struct ApeWorldVertex
{
	PLVector3 position;
	PLVectorArray *adjacentFaces;
} ApeWorldVertex;

typedef struct SSAclWorldFaceVertex
{
	PLVector2 uv;
	PLVector3 normal;
	PLColour colour;
	float lightmapU, lightmapV;

	ApeWorldVertex *u;
} SSAclWorldFaceVertex;

#define APE_WORLD_FACE_FLAG_SKY        0x01
#define APE_WORLD_FACE_FLAG_MIRRORED   0x02
#define APE_WORLD_FACE_FLAG_LIQUID     0x04
#define APE_WORLD_FACE_FLAG_DETAIL     0x08
#define APE_WORLD_FACE_FLAG_SCROLL     0x10
#define APE_WORLD_FACE_FLAG_FULLBRIGHT 0x20
#define APE_WORLD_FACE_FLAG_ALPHA      0x40
#define APE_WORLD_FACE_FLAG_HOLES      0x80
#define APE_WORLD_FACE_FLAG_LIGHTMAP   0x0300
#define APE_WORLD_FACE_FLAG_INVISIBLE  0x2000

typedef struct SSAclWorldFace
{
	float offset;
	PLVector3 normal;
	PLVector3 origin;

	int32_t smoothingGroup;

	SSAclWorldPortal *portal;

	struct ApeMaterial *material;
	int materialIndex;// index into world's material list

	PLVectorArray *vertices;// ApeWorldFaceVertex
	PLLinkedList *edgeLoop; // ApeWorldFaceVertex

	unsigned int flags; /* portal, mirror, skip etc. */

	PLCollisionAABB bounds;
} SSAclWorldFace;

typedef struct SSAclWorldMesh
{
	char id[ WORLD_PROP_TAG_LENGTH ];

	struct ApeMaterial **materials;
	unsigned int numMaterials;

	ApeWorldVertex *vertices;
	unsigned int numVertices;
	unsigned int maxVertices;

	PLLinkedList *faces;

	PLCollisionAABB bounds;

	struct PLGMesh *drawMesh; /* what actually gets rendered */

	PLLinkedListNode *node;

	ApeMemoryReference mem;
} SSAclWorldMesh;

typedef struct ApeWorldObject
{
	SSAclWorldMesh *mesh; /* pointer to mesh in worldMeshes list */

	union
	{
		const SSAclWorldMesh *collisionMesh;
		const PLCollisionAABB *collisionBounds;
	} collisionPtr;
} ApeWorldObject;

typedef struct SSAclWorldPortal
{
	PLVector3 mins;
	PLVector3 maxs;

	SSAclWorldRoom *roomA;
	SSAclWorldRoom *roomB;

	bool canSeeThrough;
} SSAclWorldPortal;

typedef struct SSAclWorldRoom
{
	char tag[ WORLD_PROP_TAG_LENGTH ];
	int uid;

	bool isDetail;
	bool containsLiquid;

	unsigned int flags;

	PLColourF32 colour;// an identifying colour
	PLColourF32 ambientLight;

	float life;

	struct
	{
		float depth;
		PLColourF32 colour;
		float visibility;
		int type;
		int alpha;
		bool plankton;
		int ppmU, ppmV;
		float angle;
		int waveform;
		float panU, panV;
	} liquid;

	PLVectorArray *detailRooms;// ApeWorldRoom
	PLVectorArray *portals;    // ApeWorldPortal
	PLVectorArray *faces;      // ApeWorldFace

	PLGMesh *mesh;    // cached mesh
	bool isMeshCached;// if false, mesh cache will be updated

	PLLinkedList *actors;// Actors currently in this sector
	PLLinkedList *lights;// Lights in this sector

	ApeAudioReverbPreset reverbPreset;

	PLCollisionAABB bounds;
} SSAclWorldRoom;

typedef struct ApeWorld
{
	char *name;
	PLPath path;

	PLVectorArray *meshes;

	PLLinkedList *entitySpawns;

	PLVector3 startPosition;
	PLMatrix3 startOrientation;

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

typedef struct ApeWorldEntity
{
	char className[ ACL_ENTITY_MAX_NAME ];
	NdBranch *properties;
} ApeWorldEntity;

PL_EXTERN_C

ApeWorld *ss_acl_world_load_rfl_file_( const char *path );

SSAclWorldRoom *ss_acl_room_create( void );
void ss_acl_room_destroy( SSAclWorldRoom *room );
SSAclWorldFace **ss_acl_room_get_faces( SSAclWorldRoom *room, unsigned int *numFaces );
SSAclWorldRoom **ss_acl_room_get_detail_rooms( SSAclWorldRoom *room, unsigned int *numDetailRooms );

void ss_acl_world_serialize_( const ApeWorld *world, NdBranch *root );

/// Deserialize world from a node tree.
/// \param world World that deserialized data will be added to.
/// \param root Handle to the world root.
/// \return On success, returns the world pointer, otherwise null.
ApeWorld *ss_acl_world_deserialize_( NdBranch *root );

void ss_acl_world_spawn_entities_( ApeWorld *world );

void ss_acl_register_level_console_variables_( void );

void ss_acl_level_client_tick_( void );

PL_EXTERN_C_END
