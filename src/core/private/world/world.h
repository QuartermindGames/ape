// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_physics.h>
#include <plcore/pl_array_vector.h>

#include <plgraphics/plg_mesh.h>

#include <yin/core_world.h>

#include "ape_memory.h"
#include "node/node_entity.h"
#include "audio/audio.h"
#include "renderer/renderer.h"

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

typedef enum ApeWorldFaceFlag : uint32_t
{
	PL_BITFLAG( APE_WORLD_FACE_FLAG_SKY, 0 ),
	PL_BITFLAG( APE_WORLD_FACE_FLAG_MIRRORED, 1 ),
} ApeWorldFaceFlag;

typedef struct ApeWorldFace
{
	float     offset;
	PLVector3 normal;
	PLVector3 origin;

	ApeWorldPortal *portal;

	ApeMaterial *material;
	int          materialIndex;// index into world's material list

	PLVectorArray *vertices;// ApeWorldFaceVertex
	PLLinkedList  *edgeLoop;// ApeWorldFaceVertex

	ApeWorldFaceFlag flags; /* portal, mirror, skip etc. */

	PLCollisionAABB bounds;
} ApeWorldFace;

typedef struct ApeWorldMesh
{
	char id[ WORLD_PROP_TAG_LENGTH ];

	ApeMaterial **materials;
	unsigned int  numMaterials;

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

	PLPath       path;
	unsigned int flags;

	PLColourF32 colour;// an identifying colour
	PLColourF32 ambientLight;

	PLVectorArray *detailRooms;// ApeWorldRoom
	PLVectorArray *portals;    // ApeWorldPortal
	PLVectorArray *faces;      // ApeWorldFace

	PLGMesh *mesh;   // cached mesh
	bool     isDirty;// if false, mesh cache will be updated

	PLLinkedList *actors;// Actors currently in this sector
	PLLinkedList *lights;// Lights in this sector

	ApeAudioReverbPreset reverbPreset;

	unsigned int numVisits;

#if !defined( APE_NO_EDITOR )
	PLPath savePath;
#endif
} ApeRoom;

typedef struct ApeWorldEntity
{
	char       className[ APE_ENTITY_MAX_NAME ];
	AcmBranch *properties;
} ApeWorldEntity;

PL_EXTERN_C

void ape_world_serialize_( const ApeWorld *world, AcmBranch *root );

void ape_world_spawn_entities_( ApeWorld *world );

/**
 * Returns the first available room childed to the world.
 *
 * @param world 	Instance of the world.
 * @return 			A room instance on success, otherwise null.
 */
ApeRoom *ape_world_get_first_room_( ApeWorld *world );

void ape_register_world_console_variables_( void );

void ape_world_node_generate_bounds_( ApeWorldNode *root );

void ape_world_draw_( ApeCamera *camera, ApeLight *light, ApeRendererPassFlag stage );
void ape_world_draw_stencil_shadows_( ApeCamera *camera, ApeLight *light );
void ape_world_draw_wireframe_( ApeWorld *world, ApeCamera *camera );

void ape_room_draw_selected_( ApeRoom *room, ApeEditorInstance *instance );

/////////////////////////////////////////////////////////////////////////////////////
// Brushes

void ape_brush_flip_face_( ApeBrushFace *face );

bool ape_brush_build_from_polygon_( ApeBrush *self, const PLVector3 *vertices, uint numVertices, PLVector3 dir, float scale, float signedArea, ApeMaterial *material );

/////////////////////////////////////////////////////////////////////////////////////
// Room

void ape_room_draw_( ApeRoom *room, ApeCamera *camera, const ApeViewport *viewport );

PL_EXTERN_C_END
