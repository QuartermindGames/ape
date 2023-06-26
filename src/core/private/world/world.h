// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_array_vector.h>

#include <plgraphics/plg_mesh.h>

#include "ape_memory_manager.h"

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
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.1f, 0.1f, 0.1f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#	define WORLD_DEFAULT_SKY         "materials/sky/cloudlayer00.mat.n"
#	define WORLD_DEFAULT_SUNPOSITION PLVector3( 0.5f, -1.0f, 0.5f )
#	define WORLD_DEFAULT_SUNCOLOUR   PL_COLOURF32( 0.0f, 0.0f, 0.0f, 0.0f )
#endif

typedef struct ApeWorldRoom ApeWorldRoom;
typedef struct ApeWorldFaceVertex ApeWorldFaceVertex;
typedef struct ApeWorldFace ApeWorldFace;
typedef struct ApeWorldMesh ApeWorldMesh;
typedef struct ApeWorldPortal ApeWorldPortal;

typedef struct ApeWorldVertex
{
	PLVector3 position;
	PLVector3 normal;
	PLVector2 uv;
	PLColourF32 colour;
	PLVectorArray *adjacentFaces;
} ApeWorldVertex;

typedef struct ApeWorldFaceVertex
{
	float textureU, textureV;
	float lightmapU, lightmapV;

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
	PLVector3 normal;
	PLVector3 origin;

	int32_t smoothingGroup;

	ApeWorldPortal *portal;

	struct ApeMaterial *material;
	// todo: reduce the below to transform matrix???
	float materialAngle;
	PLVector2 materialOffset;
	PLVector2 materialScale;

	PLVectorArray *vertices;// ApeWorldFaceVertex
	PLLinkedList *edgeLoop; // ApeWorldFaceVertex

	uint8_t flags;          /* portal, mirror, skip etc. */

	ApeWorldMesh *parentMesh;
	ApeWorldRoom *parentSector;

	// if it's a portal
	bool isPortalClosed;       // if true, we can't see through the portal
	ApeWorldRoom *targetSector;// the sector this portal connects to

	PLCollisionAABB bounds;
} ApeWorldFace;

typedef struct ApeWorldMesh
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
} ApeWorldMesh;

typedef struct ApeWorldObject
{
	ApeWorldMesh *mesh; /* pointer to mesh in worldMeshes list */

	SGTransform transform;

	union
	{
		const ApeWorldMesh *collisionMesh;
		const PLCollisionAABB *collisionBounds;
	} collisionPtr;
} ApeWorldObject;

typedef struct ApeWorldPortal
{
	PLVector3 mins;
	PLVector3 maxs;

	ApeWorldRoom *roomA;
	ApeWorldRoom *roomB;

	bool canSeeThrough;
} ApeWorldPortal;

typedef struct ApeWorldDrawBatch
{
	uint32_t *firstSubMeshes;
	uint32_t *subMeshes;
	struct ApeMaterial *material;
	uint32_t numSubMeshes;
} ApeWorldDrawBatch;

#define APE_WORLD_ROOM_FLAG_COLD     0x2
#define APE_WORLD_ROOM_FLAG_OUTSIDE  0x4
#define APE_WORLD_ROOM_FLAG_AIRLOCK  0x8
#define APE_WORLD_ROOM_FLAG_AMBIENT  0x20
#define APE_WORLD_ROOM_FLAG_ALPHA    0x40
#define APE_WORLD_ROOM_FLAG_LIFE     0x80
#define APE_WORLD_ROOM_FLAG_PLANKTON 0x1000
#define APE_WORLD_ROOM_FLAG_UNKNOWN0 0x2000
#define APE_WORLD_ROOM_FLAG_SKY      0x40000000

typedef struct ApeWorldRoom
{
	char id[ WORLD_PROP_TAG_LENGTH ];
	int32_t uid;

	bool isDetail;
	bool containsLiquid;

	uint32_t flags;

	PLColour ambientLight;

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

	PLVectorArray *detailRooms;// ApeWorldRoom
	PLVectorArray *portals;    // ApeWorldPortal
	PLVectorArray *faces;      // ApeWorldFace

	PLGMesh *mesh;             // cached mesh
	bool isMeshCached;             // if false, mesh cache will be updated
	uint32_t numBatches;

	ApeWorldObject *staticObjects;
	unsigned int numStaticObjects;

	PLLinkedList *actors;// Actors currently in this sector
	PLLinkedList *lights;// Lights in this sector

	PLCollisionAABB bounds;
} ApeWorldRoom;

#define APE_MAX_SKY_LAYERS 4

typedef struct ApeWorld
{
	char *name;
	PLPath path;

	PLVectorArray *meshes;

	PLLinkedList *entities;

	PLVector3 startPosition;
	PLMatrix3 startOrientation;

	PLVectorArray *materials;// ApeMaterial
	PLVectorArray *rooms;    // ApeWorldRoom
	PLVectorArray *portals;  // ApeWorldPortal
	PLVectorArray *vertices; // ApeWorldVertex
	PLVectorArray *faces;    // ApeWorldFace
	PLVectorArray *lights;   // ApeLight

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
} ApeWorld;

typedef struct ApeWorldEntity
{
	const ApeEntityPrefab *entityTemplate;
	NdBranch *properties;
} ApeWorldEntity;

#include <yin/core_world.h>

PL_EXTERN_C

ApeWorldRoom *apeCreateWorldRoom( void );
void apeDestroyWorldRoom( ApeWorldRoom *room );

void apeSerializeWorld( const ApeWorld *world, NdBranch *root );
ApeWorld *apeDeserializeWorld( NdBranch *root, ApeWorld *out );

ApeWorldMesh *YnCore_WorldDeserialiser_BeginMesh( NdBranch *root, ApeWorldMesh *worldMesh );

void apeSpawnWorldEntities( ApeWorld *world );

unsigned int *apeConvertWorldFaceToTriangles( const ApeWorldFace *face, unsigned int *numTriangles );

void apeRegisterWorldConsoleVariables_( void );

PL_EXTERN_C_END
