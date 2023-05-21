// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_array_vector.h>

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

typedef struct OgeWorldSector OgeWorldSector;
typedef struct OgeWorldFace OgeWorldFace;
typedef struct OgeWorldMesh OgeWorldMesh;

typedef struct OgeWorldFace
{
	PLVector3 normal;
	PLVector3 origin;

	struct OgeMaterial *material;
	// todo: reduce the below to transform matrix???
	float materialAngle;
	PLVector2 materialOffset;
	PLVector2 materialScale;

	unsigned int vertices[ WORLD_FACE_MAX_SIDES ];
	uint8_t numVertices;

	uint8_t flags; /* portal, mirror, skip etc. */

	OgeWorldMesh *parentMesh;
	OgeWorldSector *parentSector;

	// if it's a portal
	bool isPortalClosed;           // if true, we can't see through the portal
	OgeWorldSector *targetSector;  // the sector this portal connects to
	OgeWorldFace *targetSectorFace;// the 'door' on the other side

	PLCollisionAABB bounds;
} OgeWorldFace;

typedef struct OgeWorldVertex
{
	PLVector3 position;
	PLVector3 normal;
	PLVector2 uv;
	PLColourF32 colour;
} OgeWorldVertex;

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

	PLGMesh *drawMesh; /* what actually gets rendered */

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

typedef struct OgeWorldSector
{
	char id[ WORLD_PROP_TAG_LENGTH ];

	OgeWorldMesh *mesh;

	OgeWorldObject *staticObjects;
	unsigned int numStaticObjects;

	PLLinkedList *actors;// Actors currently in this sector
	PLLinkedList *lights;// Lights in this sector

	PLCollisionAABB bounds;
} OgeWorldSector;

#define OGE_MAX_SKY_LAYERS 4

typedef struct OgeWorld
{
	PLPath path;

	PLVectorArray *meshes;

	PLLinkedList *entities;

	OgeWorldSector *sectors;
	unsigned int numSectors;

	PLColourF32 ambience;
	PLColourF32 sunColour;
	PLVector3 sunPosition;

	PLColourF32 clearColour;

	PLColourF32 fogColour;
	float fogNear;
	float fogFar;

	struct OgeMaterial *skyMaterials[ OGE_MAX_SKY_LAYERS ];
	unsigned int numSkyMaterials;

	/* additional generic properties */
	struct YNNodeBranch *globalProperties;

	uint64_t lastSaveTime;
	bool isDirty;
} OgeWorld;

typedef struct OgeWorldEntity
{
	const YNCoreEntityPrefab *entityTemplate;
	YNNodeBranch *properties;
} OgeWorldEntity;

#include <yin/core_world.h>

void YnCore_WorldSerialiser_Begin( const OgeWorld *world, YNNodeBranch *root );
OgeWorld *YnCore_WorldDeserialiser_Begin( YNNodeBranch *root, OgeWorld *out );

OgeWorldMesh *YnCore_WorldDeserialiser_BeginMesh( YNNodeBranch *root, OgeWorldMesh *worldMesh );

PLLinkedList *YnCore_World_GetLights( const OgeWorld *world );
PLLinkedList *YnCore_World_GetSectorLights( const OgeWorldSector *sector );

void YnCore_World_SpawnEntities( OgeWorld *world );

bool YnCore_World_IsFaceVisible( OgeWorldFace *face, const OgeCamera *camera );
unsigned int *YnCore_World_ConvertFaceToTriangles( const OgeWorldFace *face, unsigned int *numTriangles );
bool YnCore_World_IsFacePortal( const OgeWorldFace *face );

OgeWorldSector *ogeWorld_GetSectorByNum( OgeWorld *world, int sectorNum );
