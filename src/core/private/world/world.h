// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <plcore/pl_array_vector.h>
#include <plgraphics/plg_mesh.h>

#include "ape/ape_public_world.h"

#include "node/node_entity.h"
#include "audio/audio.h"
#include "camera/camera.h"

typedef struct ApeLightGrid     ApeLightGrid;
typedef struct ApeLightGridCell ApeLightGridCell;

typedef struct ApeLightmapPixel ApeLightmapPixel;
typedef struct ApeLightmap      ApeLightmap;

//TODO: this'll get removed and we'll move over to flexible pages in future :)
static constexpr unsigned int APE_ROOM_MAX_LIGHTMAPS = 4;

typedef struct ApeRoom
{
	// This should always come first!
	ApeWorldNode base;

	unsigned int flags;

	ApeColour4fProperty ambientLight;

	ApeColour4fProperty fogColour;
	ApeFloatProperty    fogNear;
	ApeFloatProperty    fogFar;

	struct PLHashTable *taggedSurfaceLookup;

	ApeAudioReverbPreset reverbPreset;// default reverb for the room
	QmMathVector3f       gravity;     // default gravity for the room

	unsigned int numVisits;

	ApeDecalManager *decalManager;

	ApeLightGrid *lightGrid;                          // 3d grid for querying lighting values
	unsigned int  lightmapEdgeLength;                 // represents w and h value
	ApeLightmap  *lightmaps[ APE_ROOM_MAX_LIGHTMAPS ];// lightmap buffers
	unsigned int  numLightmaps;
} ApeRoom;

PL_EXTERN_C

void ape_world_serialize_( const ApeWorld *world, AcmBranch *root );
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

void ape_world_draw_stencil_shadows_( ApeCamera *camera, const ApeLight *light );
void ape_world_draw_wireframe_( ApeWorld *world, ApeCamera *camera );

/////////////////////////////////////////////////////////////////////////////////////
// Nodes

ApeWorldNode *ape_world_node_setup_( ApeWorldNode *self, ApeWorldNode *parent, ApeWorldNodeType type, const char *name, const QmMathVector3f *position, const QmMathVector3f *angles );

void     ape_world_node_mark_dirty_( ApeWorldNode *self );
PLGMesh *ape_world_node_get_mesh_( ApeWorldNode *self );
void     ape_world_node_update_mesh_cache_( ApeWorldNode *self );

/////////////////////////////////////////////////////////////////////////////////////
// Rooms

void ape_room_draw_selected_( ApeRoom *room, ApeEditorInstance *instance );

/////////////////////////////////////////////////////////////////////////////////////
// Brushes

void ape_brush_flip_face_( ApeBrushFace *face );

bool ape_brush_build_from_polygon_( ApeBrush *self, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f dir, float scale, float signedArea, ApeMaterial *material, ApeEditorBrushType type );

/////////////////////////////////////////////////////////////////////////////////////
// Room

void ape_room_draw_( ApeCamera *camera, ApeCameraVisibleRoom *visibleRoom, const ApeViewport *viewport );

PL_EXTERN_C_END
