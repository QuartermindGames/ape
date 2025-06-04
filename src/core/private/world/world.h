// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_array_vector.h>

#include <plgraphics/plg_mesh.h>

#include "yin/core_world.h"
#include "node/node_entity.h"
#include "audio/audio.h"
#include "camera/camera.h"

#define WORLD_PROP_TAG_LENGTH 64

#if 1 /* original values, used for prototype */
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.25f, 0.25f, 0.25f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.1f, 0.5f, 1.0f, 1.0f )
#else
#	define WORLD_DEFAULT_AMBIENCE    PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#	define WORLD_DEFAULT_CLEARCOLOUR PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f )
#endif

typedef struct ApeWorldVertex
{
	PLVector3      position;
	PLVector3      colour;
	PLVectorArray *adjacentFaces;
} ApeWorldVertex;

typedef struct ApeRoomZone
{
	// we're not using our typical AABB type here, just because these are *always* absolute!
	PLVector3 mins, maxs;

	PLLinkedList *worldNodes;
	PLLinkedList *portals;
} ApeRoomZone;

typedef struct ApeRoom
{
	// This should always come first!
	ApeWorldNode base;

	PLPath       path;
	unsigned int flags;

	PLColourF32 colour;// an identifying colour
	PLColourF32 ambientLight;

	struct PLHashTable *taggedSurfaceLookup;

	ApeAudioReverbPreset reverbPreset;// default reverb for the room
	PLVector3            gravity;     // default gravity for the room

	unsigned int numVisits;

#if !defined( APE_NO_EDITOR )
	PLPath savePath;
#endif
} ApeRoom;

PL_EXTERN_C

void ape_world_serialize_( const ApeWorld *world, AcmBranch *root );

void ape_world_spawn_entities_( ApeWorld *self );
void ape_world_tick_entities_( ApeWorld *self, double delta );

/**
 * Returns the first available room childed to the world.
 *
 * @param world 	Instance of the world.
 * @return 			A room instance on success, otherwise null.
 */
ApeRoom *ape_world_get_first_room_( ApeWorld *world );

void ape_register_world_console_variables_( void );

void ape_world_node_compute_bounds_( ApeWorldNode *self );

void ape_world_draw_stencil_shadows_( ApeCamera *camera, ApeLight *light );
void ape_world_draw_wireframe_( ApeWorld *world, ApeCamera *camera );

/////////////////////////////////////////////////////////////////////////////////////
// Nodes

ApeWorldNode *ape_world_node_setup_( ApeWorldNode *self, ApeWorldNode *parent, ApeWorldNodeType type, const char *name, const PLVector3 *position, const PLVector3 *angles );

void     ape_world_node_mark_dirty_( ApeWorldNode *self );
PLGMesh *ape_world_node_get_mesh_( ApeWorldNode *self );
void     ape_world_node_update_mesh_cache_( ApeWorldNode *self );

/////////////////////////////////////////////////////////////////////////////////////
// Rooms

void ape_room_draw_selected_( ApeRoom *room, ApeEditorInstance *instance );

/////////////////////////////////////////////////////////////////////////////////////
// Brushes

void ape_brush_flip_face_( ApeBrushFace *face );

bool ape_brush_build_from_polygon_( ApeBrush *self, const PLVector3 *vertices, unsigned int numVertices, PLVector3 dir, float scale, float signedArea, ApeMaterial *material );

/////////////////////////////////////////////////////////////////////////////////////
// Room

void ape_room_draw_( ApeCamera *camera, ApeCameraVisibleRoom *visibleRoom, const ApeViewport *viewport );

PL_EXTERN_C_END
