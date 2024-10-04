// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "common.h"

#include "yin/core_world.h"

PL_EXTERN_C

typedef enum ApeEditorMode
{
	APE_EDITOR_MODE_INVALID,
	APE_EDITOR_MODE_WORLD,
	APE_EDITOR_MODE_MODEL,
	APE_EDITOR_MODE_VECTOR,

	APE_EDITOR_MAX_MODES
} ApeEditorMode;

#define APE_EDITOR_MAX_VIEWPORTS      1
#define APE_EDITOR_MAX_VIEW_BOOKMARKS 16

typedef struct ApeEditorField
{
	char        name[ 64 ];
	char        description[ 128 ];
	ComDataType type;
	uintptr_t   varOffset;
} ApeEditorField;

#define APE_ENTITY_COMPONENT_BEGIN_PROPERTIES() static ApeEditorField x_editorVariables[] = {
#define APE_ENTITY_COMPONENT_END_PROPERTIES() \
	}                                         \
	;                                         \
	static unsigned int x_numEditorVariables = PL_ARRAY_ELEMENTS( x_editorVariables );
#define APE_ENTITY_COMPONENT_PROPERTY( TYPE, VAR, DESC, VARTYPE ) \
	{ #VAR, DESC, VARTYPE, PL_OFFSETOF( TYPE, VAR ) },
#define APE_ENTITY_HOOK_PROPERTIES( CBTABLE )        \
	( CBTABLE ).editorFields    = x_editorVariables; \
	( CBTABLE ).numEditorFields = x_numEditorVariables

typedef enum ApeEditorGeometryMode
{
	APE_EDITOR_GEOMETRY_MODE_PLOT,     // brush creation mode
	APE_EDITOR_GEOMETRY_MODE_FACE,     // face selection mode
	APE_EDITOR_GEOMETRY_MODE_VERTEX,   // vertex selection mode
	APE_EDITOR_GEOMETRY_MODE_TRANSFORM,// object transform mode

	APE_EDITOR_MAX_GEOMETRY_MODES
} ApeEditorGeometryMode;

#define APE_EDITOR_GRID_MAX_SIZE   256
#define APE_EDITOR_GRID_MAX_POINTS ( APE_EDITOR_GRID_MAX_SIZE * APE_EDITOR_GRID_MAX_SIZE )

typedef struct ApeEditorGrid
{
	unsigned int scale;
	PLMatrix4    transform;

	bool     rebuildMesh;
	PLGMesh *mesh;

	unsigned char visible;// unsigned char, because otherwise
	                      // can't hook it with frontend :(
} ApeEditorGrid;

typedef struct ApeEditorInstance
{
	ApeCamera *camera;

	ApeEditorMode         mode;
	ApeEditorGeometryMode geometryMode;
	ApeEditorGrid         grid;

	float forwardSpeed;
	float turnSpeed;

	// polygon points are always interpreted on a 2D plane for simplicity’s sake
	PLVector2 polygonPoints[ APE_BRUSH_MAX_FACE_VERTICES ];
	uint      numPolygonPoints;

	void *modeData;

	struct PLHashTable *selectionTable;
	union
	{
		void         *selectedObject;
		ApeBrushFace *selectedFace;
	};
	ApeWorldNode *currentNode;//todo: there is some overlap with the above...

	struct PLLinkedListNode *listNode;// index in the table of instances
	bool                     managed; // indicates the engine manages the instance
} ApeEditorInstance;

ApeEditorInstance *ape_editor_instance_setup( ApeEditorInstance *self, ApeEditorMode mode );
void               ape_editor_instance_cleanup( ApeEditorInstance *self );
void               ape_editor_set_active_instance( ApeEditorInstance *self );
ApeEditorInstance *ape_editor_get_active_instance( void );

void ape_editor_set_geometry_mode( ApeEditorInstance *self, ApeEditorGeometryMode geometryMode );

ApeMaterial **ape_editor_get_available_materials( unsigned int *numMaterials );

PLColour *ape_editor_get_pixel_under_cursor( PLColour *dst );
void     *ape_editor_get_object_under_cursor( ApeEditorInstance *self );

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

PLVector2 *ape_grid_get_cursor_position( ApeEditorGrid *self, PLVector2 *dst );

/**
 * Transforms a 2D point into 3D space using the grid's transformation matrix.
 *
 * @param self 	A pointer to an ApeEditorGrid structure containing transformation information.
 * @param point A pointer to a PLVector2 structure representing the 2D point to be transformed.
 * @return 		A PLVector3 structure representing the transformed 3D point.
 */
PLVector3 ape_grid_transform_point( ApeEditorGrid *self, const PLVector2 *point );

void ape_grid_increase_size( void );
void ape_grid_decrease_size( void );
uint ape_grid_get_size( ApeEditorGrid *self );
void ape_grid_set_visibility( ApeEditorGrid *self, bool visible );

void ape_grid_align_to_face( ApeEditorGrid *self, ApeBrushFace *face );

/////////////////////////////////////////////////////////////////////////////////////
// Brush Plotting
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Creates an `ApeBrush` from the polygon points stored in the `ApeEditorInstance`.
 *
 * @param self 	Pointer to the `ApeEditorInstance` which holds the polygon points and other relevant data.
 * @return 		A pointer to the newly created `ApeBrush`, or `nullptr` if the brush could not be created.
 */
ApeBrush *ape_editor_brush_from_polygon( ApeEditorInstance *self );

/**
 * This function decreases the number of points in the polygon managed by the
 * given `ApeEditorInstance`. If there are no points in the polygon (`numPolygonPoints` is 0),
 * the function does nothing.
 *
 * @param self A pointer to the `ApeEditorInstance` from which the last polygon point will be removed.
 */
void ape_editor_remove_polygon_point( ApeEditorInstance *self );

/**
 * Plots a new point for a brush. Keep in mind this is merely triggering it to occur,
 * and it will operate off whatever is the current grid point.
 *
 * @param self 	The editor state that the action is being performed within.
 * @return 			True if the point is at the same location as the origin.
 */
bool ape_editor_add_polygon_point( ApeEditorInstance *self );

void ape_editor_clear_plot_points( ApeEditorInstance *instance );

PL_EXTERN_C_END
