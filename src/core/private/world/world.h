// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_array_vector.h>

#include "core_memory_manager.h"

#include "client/renderer/renderer_scenegraph.h"
#include "entity/entity.h"

#define WORLD_PROP_TAG_LENGTH   64
#define WORLD_PROP_VALUE_LENGTH 256

#if 0 /* original values, used for prototype */
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.4f, 0.4f, 0.4f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#	define WORLD_DEFAULT_SKY         "materials/sky/cloudlayer00.mat.n"
#	define WORLD_DEFAULT_SUNPOSITION PLVector3( 0.5f, -1.0f, 0.5f )
#	define WORLD_DEFAULT_SUNCOLOUR   PL_COLOURF32( 1.0f, 1.0f, 1.0f, 1.25f )
#else
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#	define WORLD_DEFAULT_SKY         "materials/sky/cloudlayer00.mat.n"
#	define WORLD_DEFAULT_SUNPOSITION PLVector3( 0.5f, -1.0f, 0.5f )
#	define WORLD_DEFAULT_SUNCOLOUR   PL_COLOURF32( 0.0f, 0.0f, 0.0f, 0.0f )
#endif

enum OgeWorldFaceFlag
{
	PL_BITFLAG( WORLD_FACE_FLAG_PORTAL, 0U ), /* reflect portal */
	PL_BITFLAG( WORLD_FACE_FLAG_MIRROR, 1U ), /* reflect back */
	PL_BITFLAG( WORLD_FACE_FLAG_SKIP, 2U ),   /* skip face */
};

typedef enum OgeWorldObjectCollisionType
{
	WORLD_OBJECT_COLLISION_POLY,
	WORLD_OBJECT_COLLISION_SPHERE,
	WORLD_OBJECT_COLLISION_AABB,
} OgeWorldObjectCollisionType;

#define WORLD_FACE_MAX_SIDES 32

typedef struct OgeWorldRoom OgeWorldRoom;
typedef struct OgeWorldFaceVertex OgeWorldFaceVertex;
typedef struct OgeWorldFace OgeWorldFace;
typedef struct OgeWorldMesh OgeWorldMesh;
typedef struct OgeWorldPortal OgeWorldPortal;

typedef struct OgeWorldVertex
{
	PLVector3 position;
	PLVector3 normal;
	PLVector2 uv;
	PLColourF32 colour;
	PLVectorArray *adjacentFaces;
} OgeWorldVertex;

typedef struct OgeWorldFaceVertex
{
	float textureU, textureV;
	float lightmapU, lightmapV;

	OgeWorldVertex *u;
} OgeWorldFaceVertex;

#define OGE_WORLD_FACE_FLAG_SKY        0x01
#define OGE_WORLD_FACE_FLAG_MIRRORED   0x02
#define OGE_WORLD_FACE_FLAG_LIQUID     0x04
#define OGE_WORLD_FACE_FLAG_DETAIL     0x08
#define OGE_WORLD_FACE_FLAG_SCROLL     0x10
#define OGE_WORLD_FACE_FLAG_FULLBRIGHT 0x20
#define OGE_WORLD_FACE_FLAG_ALPHA      0x40
#define OGE_WORLD_FACE_FLAG_HOLES      0x80
#define OGE_WORLD_FACE_FLAG_LIGHTMAP   0x0300
#define OGE_WORLD_FACE_FLAG_INVISIBLE  0x2000

typedef struct OgeWorldFace
{
	PLVector3 normal;
	PLVector3 origin;

	OgeWorldPortal *portal;

	struct OgeMaterial *material;
	// todo: reduce the below to transform matrix???
	float materialAngle;
	PLVector2 materialOffset;
	PLVector2 materialScale;

	PLVectorArray *vertices;// OgeWorldFaceVertex
	PLLinkedList *edgeLoop; // OgeWorldFaceVertex

	uint8_t flags;          /* portal, mirror, skip etc. */

	OgeWorldMesh *parentMesh;
	OgeWorldRoom *parentSector;

	// if it's a portal
	bool isPortalClosed;       // if true, we can't see through the portal
	OgeWorldRoom *targetSector;// the sector this portal connects to

	PLCollisionAABB bounds;
} OgeWorldFace;

typedef struct OgeWorldMesh
{
	char id[ WORLD_PROP_TAG_LENGTH ];

	struct OgeMaterial **materials;
	unsigned int numMaterials;

	OgeWorldVertex *vertices;
	unsigned int numVertices;
	unsigned int maxVertices;

	PLLinkedList *faces;

	PLCollisionAABB bounds;

	struct PLGMesh *drawMesh; /* what actually gets rendered */

	PLLinkedListNode *node;

	OgeMemoryReference mem;
} OgeWorldMesh;

typedef struct OgeWorldObject
{
	OgeWorldMesh *mesh; /* pointer to mesh in worldMeshes list */

	SGTransform transform;

	OgeWorldObjectCollisionType collisionType;
	union
	{
		const OgeWorldMesh *collisionMesh;
		const PLCollisionAABB *collisionBounds;
	} collisionPtr;
} OgeWorldObject;

typedef struct OgeWorldPortal
{
	PLVector3 mins;
	PLVector3 maxs;

	OgeWorldRoom *roomA;
	OgeWorldRoom *roomB;

	bool canSeeThrough;
} OgeWorldPortal;

typedef struct OgeWorldRoom
{
	char id[ WORLD_PROP_TAG_LENGTH ];
	int32_t uid;

	bool isSky;
	bool isCold;
	bool isOutside;
	bool isAirLock;
	bool containsLiquid;

	bool ambientLightDefined;
	PLColour ambientLight;

	bool hasAlpha;
	bool isDetail;
	bool isInvincible;

	float life;

	struct
	{
		float depth;
		PLColour colour;
		float visibility;
		int32_t type;
		int32_t alpha;
		bool plankton;
		int32_t ppmU, ppmV;
		float angle;
		int32_t waveform;
		float panU, panV;
	} liquid;

	PLVectorArray *detailRooms;// OgeWorldRoom
	PLVectorArray *portals;    // OgeWorldPortal
	PLVectorArray *faces;      // OgeWorldFace

	OgeWorldMesh *mesh;

	OgeWorldObject *staticObjects;
	unsigned int numStaticObjects;

	PLLinkedList *actors;// Actors currently in this sector
	PLLinkedList *lights;// Lights in this sector

	PLCollisionAABB bounds;
} OgeWorldRoom;

OgeWorldRoom *ogeCreateWorldRoom( void );
void ogeDestroyWorldRoom( OgeWorldRoom *room );

#define OGE_MAX_SKY_LAYERS 4

typedef struct OgeWorld
{
	char *name;
	PLPath path;

	PLVectorArray *meshes;

	PLLinkedList *entities;

	PLVectorArray *materials;// OgeMaterial
	PLVectorArray *rooms;    // OgeWorldRoom
	PLVectorArray *portals;  // OgeWorldPortal
	PLVectorArray *vertices; // OgeWorldVertex
	PLVectorArray *faces;    // OgeWorldFace

	PLColourF32 ambience;
	PLColourF32 sunColour;
	PLVector3 sunPosition;

	PLColourF32 clearColour;

	PLColourF32 fogColour;
	float fogNear;
	float fogFar;

	PLCollisionAABB bounds;

	/* additional generic properties */
	struct NdBranch *globalProperties;

	uint64_t lastSaveTime;
	bool isDirty;
} OgeWorld;

typedef struct OgeWorldEntity
{
	const YNCoreEntityPrefab *entityTemplate;
	NdBranch *properties;
} OgeWorldEntity;

#include <yin/core_world.h>

void YnCore_WorldSerialiser_Begin( const OgeWorld *world, NdBranch *root );
OgeWorld *YnCore_WorldDeserialiser_Begin( NdBranch *root, OgeWorld *out );

OgeWorldMesh *YnCore_WorldDeserialiser_BeginMesh( NdBranch *root, OgeWorldMesh *worldMesh );

PLLinkedList *YnCore_World_GetLights( const OgeWorld *world );
PLLinkedList *YnCore_World_GetSectorLights( const OgeWorldRoom *sector );

void ogeWorld_SpawnEntities( OgeWorld *world );

bool YnCore_World_IsFaceVisible( OgeWorldFace *face, const OgeCamera *camera );
unsigned int *ogeWorld_ConvertFaceToTriangles( const OgeWorldFace *face, unsigned int *numTriangles );
bool YnCore_World_IsFacePortal( const OgeWorldFace *face );

OgeWorldRoom *ogeWorld_GetSectorByNum( OgeWorld *world, int sectorNum );
