// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_array_vector.h>

#include <plgraphics/plg_mesh.h>

#include <yin/core_world.h>

#include "ape_memory_manager.h"
#include "entity/entity.h"

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

typedef struct ApeWorldRoom ApeWorldRoom;
typedef struct ApeWorldFaceVertex ApeWorldFaceVertex;
typedef struct ApeWorldFace ApeWorldFace;
typedef struct ApeWorldMesh ApeWorldMesh;
typedef struct ApeWorldPortal ApeWorldPortal;

typedef struct ApeWorldVertex
{
	PLVector3 position;
	PLVectorArray *adjacentFaces;
} ApeWorldVertex;

typedef struct ApeWorldFaceVertex
{
	PLVector2 uv;
	PLVector3 normal;
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
	float offset;
	PLVector3 normal;
	PLVector3 origin;

	int32_t smoothingGroup;

	ApeWorldPortal *portal;

	struct ApeMaterial *material;
	int materialIndex;// index into world's material list

	PLVectorArray *vertices;// ApeWorldFaceVertex
	PLLinkedList *edgeLoop; // ApeWorldFaceVertex

	unsigned int flags; /* portal, mirror, skip etc. */

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

typedef struct ApeWorldRoom
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

	PLGMesh *mesh;      // cached mesh
	PLGMesh *shadowMesh;// shadow volume mesh
	bool isMeshCached;  // if false, mesh cache will be updated

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

ApeWorld *apeParseRFWorld_( PLFile *file );

ApeWorldRoom *apeCreateWorldRoom( void );
void apeDestroyWorldRoom( ApeWorldRoom *room );
ApeWorldFace **apeGetWorldRoomFaces( ApeWorldRoom *room, unsigned int *numFaces );

void apeSerializeWorld( const ApeWorld *world, NdBranch *root );
ApeWorld *acl_world_deserialize( ApeWorld *world, NdBranch *root );

ApeWorldMesh *YnCore_WorldDeserialiser_BeginMesh( NdBranch *root, ApeWorldMesh *worldMesh );

void apeSpawnWorldEntities( ApeWorld *world );

unsigned int *apeConvertWorldFaceToTriangles( const ApeWorldFace *face, unsigned int *numTriangles );

void apeRegisterWorldConsole_( void );

void apeTickClientWorld_( void );

/////////////////////////////////////////////////////////////////
// Visibility API

void apeInitializeWorldVisibilitySystem_( void );
void apeShutdownWorldVisibilitySystem_( void );

struct SS_Arl_Light **apeGetVisibleLights_( unsigned int *num );
ApeWorldRoom **apeGetVisibleRooms_( unsigned int *num );

void apeBuildWorldVisibiltyLists_( void );
void apeFlushWorldVisibilityLists_( void );

PL_EXTERN_C_END
