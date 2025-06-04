// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

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

#define APE_EDITOR_GRID_MAX_POINTS_ROW 256
#define APE_EDITOR_GRID_MAX_POINTS     ( APE_EDITOR_GRID_MAX_POINTS_ROW * APE_EDITOR_GRID_MAX_POINTS_ROW )

typedef struct ApeEditorGrid
{
	unsigned int size;
	unsigned int sizeScale;

	PLMatrix4 transform;

	PLVector2 cursor;

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
	PLVector2      polygonPoints[ APE_BRUSH_MAX_FACE_VERTICES ];
	unsigned int   numPolygonPoints;
	PLRectangleF32 polySize;

	void *modeData;

	struct PLHashTable *selectionTable;
	struct PLHashTable *subSelectionTable;
	PLLinkedList       *selectedObjects;
	void               *hoverSelection;

	struct PLLinkedListNode *listNode;// index in the table of instances
	bool                     managed; // indicates the engine manages the instance
} ApeEditorInstance;

ApeEditorInstance *ape_editor_instance_setup( ApeEditorInstance *self, ApeEditorMode mode );
void               ape_editor_instance_cleanup( ApeEditorInstance *self );
void               ape_editor_set_active_instance( ApeEditorInstance *self );
ApeEditorInstance *ape_editor_get_active_instance( void );

AcmBranch *ape_editor_get_config();

void ape_editor_set_geometry_mode( ApeEditorInstance *self, ApeEditorGeometryMode geometryMode );

void     *ape_editor_get_object_under_cursor( ApeEditorInstance *self );
void      ape_editor_clear_selection( ApeEditorInstance *self );
void      ape_editor_add_object_to_selection( ApeEditorInstance *self, void *object );
void     *ape_editor_get_first_selected( ApeEditorInstance *self );
void      ape_editor_delete_selection( ApeEditorInstance *self );
void      ape_editor_move_selection_to_room( ApeEditorInstance *self, ApeRoom *room );

void ape_editor_toggle_faces( ApeEditorInstance *self );
void ape_editor_toggle_other_faces( ApeEditorInstance *self );
void ape_editor_flip_faces( ApeEditorInstance *self );
void ape_editor_shade_faces_smooth( ApeEditorInstance *self );
void ape_editor_shade_faces_flat( ApeEditorInstance *self );

/**
 * Attempt to duplicate the currently selected world nodes.
 * Mind that duplication doesn't work for everything.
 *
 * @param self Editor instance.
 */
void ape_editor_duplicate_selection( ApeEditorInstance *self );

/**
 * Attempt to shift the current selection in the given direction.
 * The direction will be relative to the grid.
 *
 * @param self	Editor instance.
 * @param dir	Direction to shift in.
 */
void ape_editor_shift_selection( ApeEditorInstance *self, const PLVector3 *dir );

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
PLVector3 ape_grid_transform_point( const ApeEditorGrid *self, const PLVector2 *point );

void ape_grid_increase_size( void );
void ape_grid_decrease_size( void );

void ape_grid_align_to_face( ApeEditorGrid *self, ApeBrushFace *face );

void ape_grid_move_forward( ApeEditorGrid *self );
void ape_grid_move_backward( ApeEditorGrid *self );

void ape_editor_on_mouse_move( ApeEditorInstance *self, const ApeViewport *viewport, int x, int y );

/////////////////////////////////////////////////////////////////////////////////////
// Brush Plotting
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Creates an `ApeBrush` from the polygon points stored in the `ApeEditorInstance`.
 *
 * @param self 	Pointer to the `ApeEditorInstance` which holds the polygon points and other relevant data.
 * @return 		A pointer to the newly created `ApeBrush`, or `nullptr` if the brush could not be created.
 */
ApeBrush *ape_editor_brush_from_polygon( ApeEditorInstance *self, const char *materialPath );

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
