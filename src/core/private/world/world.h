// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_array_vector.h>

#include <plgraphics/plg_mesh.h>

#include <yin/core_world.h>

#include "ape_memory.h"
#include "nodes/node_entity.h"
#include "audio/audio.h"

#define WORLD_PROP_TAG_LENGTH 64

#if 1 /* original values, used for prototype */
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.25f, 0.25f, 0.25f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.1f, 0.5f, 1.0f, 1.0f )
#else
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#endif

typedef struct PLFile PLFile;

typedef struct ApeRoom            ApeRoom;
typedef struct ApeWorldFaceVertex ApeWorldFaceVertex;
typedef struct ApeWorldFace       ApeWorldFace;
typedef struct ApeWorldMesh       ApeWorldMesh;
typedef struct ApeWorldPortal     ApeWorldPortal;

typedef struct ApeWorldVertex
{
	PLVector3      position;
	PLVector3      colour;
	PLVectorArray *adjacentFaces;
} ApeWorldVertex;

typedef struct ApeWorldFaceVertex
{
	PLVector2       uv;
	PLVector3       normal;
	ApeWorldVertex *u;
} ApeWorldFaceVertex;

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

typedef struct ApeWorldFace
{
	float     offset;
	PLVector3 normal;
	PLVector3 origin;

	ApeWorldPortal *portal;

	struct ApeMaterial *material;
	int                 materialIndex;// index into world's material list

	PLVectorArray *vertices;// ApeWorldFaceVertex
	PLLinkedList  *edgeLoop;// ApeWorldFaceVertex

	unsigned int flags; /* portal, mirror, skip etc. */

	PLCollisionAABB bounds;
} ApeWorldFace;

typedef struct ApeWorldMesh
{
	char id[ WORLD_PROP_TAG_LENGTH ];

	struct ApeMaterial **materials;
	unsigned int         numMaterials;

	ApeWorldVertex *vertices;
	unsigned int    numVertices;
	unsigned int    maxVertices;

	PLLinkedList     *faces;
	PLLinkedListNode *node;

	ApeMemoryReference mem;
} ApeWorldMesh;

typedef struct ApeWorldObject
{
	ApeWorldMesh *mesh; /* pointer to mesh in worldMeshes list */

	union
	{
		const ApeWorldMesh    *collisionMesh;
		const PLCollisionAABB *collisionBounds;
	} collisionPtr;
} ApeWorldObject;

typedef struct ApeWorldPortal
{
	PLVector3 mins;
	PLVector3 maxs;

	ApeRoom *roomA;
	ApeRoom *roomB;

	bool canSeeThrough;
} ApeWorldPortal;

typedef struct ApeRoom
{
	// This should always come first!
	ApeWorldNode base;

	bool isDetail;

	unsigned int flags;

	PLColourF32 colour;// an identifying colour
	PLColourF32 ambientLight;

	PLVectorArray *detailRooms;// ApeWorldRoom
	PLVectorArray *portals;    // ApeWorldPortal
	PLVectorArray *faces;      // ApeWorldFace

	PLGMesh *mesh;        // cached mesh
	bool     isMeshCached;// if false, mesh cache will be updated

	PLLinkedList *actors;// Actors currently in this sector
	PLLinkedList *lights;// Lights in this sector

	ApeAudioReverbPreset reverbPreset;

	unsigned int numVisits;
} ApeRoom;

typedef struct ApeWorldEntity
{
	char       className[ APE_ENTITY_MAX_NAME ];
	AcmBranch *properties;
} ApeWorldEntity;

PL_EXTERN_C

ApeWorldFace **ape_world_room_get_faces_( ApeRoom *self, unsigned int *numFaces );

void ape_world_serialize_( const ApeWorld *world, AcmBranch *root );

/// Deserialize world from a node tree.
/// \param world World that deserialized data will be added to.
/// \param root Handle to the world root.
/// \return On success, returns the world pointer, otherwise null.
ApeWorld *ape_world_deserialize_( AcmBranch *root );

void ape_world_spawn_entities_( ApeWorld *world );

void ape_register_world_console_variables_( void );

void ape_world_node_generate_bounds_( ApeWorldNode *root );

/**
 * Iterates over the children of the world and returns the first room.
 *
 * @param self	Pointer to instance of world.
 * @return 		Pointer to instance of room. Null on fail.
 */
ApeRoom *ape_world_get_first_room_( ApeWorld *self );

PL_EXTERN_C_END
