// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "aux/public/aux.h"

#include "ape_public_world.h"

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

static constexpr QmMathColour4ub APE_EDITOR_COLOUR_SELECT_BOUNDS = QM_MATH_COLOUR4UB_RGB( 0, 255, 0 );

/////////////////////////////////////////////////////////////////////////////////////
// Object Properties
/////////////////////////////////////////////////////////////////////////////////////

typedef enum ApePropertyType
{
	// DO NOT CHANGE THE ORDER OF THESE!!!
	APE_PROPERTY_TYPE_INVALID = 0,
	APE_PROPERTY_TYPE_FLOAT,
	APE_PROPERTY_TYPE_VEC2,
	APE_PROPERTY_TYPE_VEC3,
	APE_PROPERTY_TYPE_VEC4,
	APE_PROPERTY_TYPE_ENUM,
	APE_PROPERTY_TYPE_COLOUR,
	APE_PROPERTY_TYPE_INTEGER,
	APE_PROPERTY_TYPE_STRING,
	APE_PROPERTY_TYPE_PATH,
	APE_PROPERTY_TYPE_BOOLEAN,
	APE_PROPERTY_TYPE_BITFLAG,

	APE_PROPERTY_MAX_TYPES,
} ApePropertyType;

// if you decide to refactor the below,
// mind that the validation will need updating!

typedef float          ApeFloatProperty;
typedef QmMathVector2f ApeVec2Property;
typedef QmMathVector3f ApeVec3Property;
typedef QmMathVector4f ApeVec4Property;
typedef unsigned int   ApeEnumProperty;
typedef QmMathColour4f ApeColour4fProperty;
typedef int            ApeIntegerProperty;
typedef char           ApeStringProperty;
typedef bool           ApeBooleanProperty;

#if !defined( NDEBUG )
#	define APE_PROPERTY_GET_TYPENAME( X ) _Generic( ( X ), \
		    ApeFloatProperty: "ApeFloatProperty",           \
		    ApeVec2Property: "ApeVec2Property",             \
		    ApeVec3Property: "ApeVec3Property",             \
		    ApeVec4Property: "ApeVec4Property",             \
		    ApeEnumProperty: "ApeEnumProperty",             \
		    ApeColour4fProperty: "ApeColour4fProperty",     \
		    ApeIntegerProperty: "ApeIntegerProperty",       \
		    ApeStringProperty *: "ApeStringProperty",       \
		    ApeBooleanProperty: "ApeBooleanProperty",       \
		    default: "invalid" )
#endif

typedef struct ApePropertyEnum
{
	const char  *name;
	unsigned int value;
} ApePropertyEnum;

typedef struct ApeProperty
{
	const char     *name;
	const char     *internalName;
	const char     *description;
	uintptr_t       offset;
	ApePropertyType type;

	union
	{
		struct
		{
			ApePropertyEnum *enums;
			unsigned int     numEnums;
		} enumType;
		struct
		{
			unsigned int maxSize;
		} stringType;
		struct
		{
			unsigned int value;
		} bitFlagType;
	};

#if !defined( NDEBUG )
	const char *typeName;
#endif
} ApeProperty;

#if !defined( NDEBUG )
#	define APE_PROPERTY_HEADER( NAME, DESC, TYPE, VAR, PROP ) NAME, #VAR, DESC, offsetof( TYPE, VAR ), APE_PROPERTY_TYPE_##PROP, .typeName = APE_PROPERTY_GET_TYPENAME( ( ( TYPE * ) nullptr )->VAR )
#else
#	define APE_PROPERTY_HEADER( NAME, DESC, TYPE, VAR, PROP ) NAME, #VAR, DESC, offsetof( TYPE, VAR ), APE_PROPERTY_TYPE_##PROP
#endif

#define APE_PROPERTY_BASIC( NAME, DESC, TYPE, VAR, PROP ) \
	{ APE_PROPERTY_HEADER( NAME, DESC, TYPE, VAR, PROP ) }
#define APE_PROPERTY_STRING( NAME, DESC, TYPE, VAR )                 \
	{                                                                \
	        APE_PROPERTY_HEADER( NAME, DESC, TYPE, VAR, STRING ),    \
	        .stringType = { sizeof( ( ( TYPE * ) nullptr )->VAR ) }, \
	}
#define APE_PROPERTY_ENUM( NAME, DESC, TYPE, VAR, ENUMS )       \
	{                                                           \
	        APE_PROPERTY_HEADER( NAME, DESC, TYPE, VAR, ENUM ), \
	        .enumType = { ENUMS,                                \
                         QM_OS_ARRAY_ELEMENTS( ENUMS ) },      \
}
#define APE_PROPERTY_BITFLAG( NAME, DESC, TYPE, VAR, VALUE )                                    \
	{                                                                                           \
		APE_PROPERTY_HEADER( NAME, DESC, TYPE, VAR, BITFLAG ), .bitFlagType = {.value = VALUE } \
	}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

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

	QmMathVector2f cursor;

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
	QmMathVector2f polygonPoints[ APE_BRUSH_MAX_FACE_VERTICES ];
	unsigned int   numPolygonPoints;
	PLRectangleF32 polySize;

	void *modeData;

	struct PLHashTable *selectionTable;
	struct PLHashTable *subSelectionTable;
	QmOsLinkedList     *selectedObjects;
	void               *hoverSelection;
} ApeEditorInstance;

ApeEditorInstance *ape_editor_instance_setup( ApeEditorInstance *self, ApeEditorMode mode );
void               ape_editor_instance_cleanup( ApeEditorInstance *self );
void               ape_editor_set_active_instance( ApeEditorInstance *self );
ApeEditorInstance *ape_editor_get_active_instance( void );

bool ape_editor_is_active();

AcmBranch *ape_editor_get_config();

void ape_editor_set_geometry_mode( ApeEditorInstance *self, ApeEditorGeometryMode geometryMode );

QmMathVector2f ape_editor_get_default_material_scale();

void *ape_editor_get_object_under_cursor( ApeEditorInstance *self );
void  ape_editor_clear_selection( ApeEditorInstance *self );
void  ape_editor_add_object_to_selection( ApeEditorInstance *self, void *object );
void *ape_editor_get_first_selected( ApeEditorInstance *self );
void  ape_editor_delete_selection( ApeEditorInstance *self );
void  ape_editor_move_selection_to_room( ApeEditorInstance *self, ApeRoom *room );

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
void ape_editor_shift_selection( ApeEditorInstance *self, const QmMathVector3f *dir );

/**
 * Loads a preview of the specified material, rather than loading the entire thing.
 *
 * @param path	Path to the specific material.
 * @param width
 * @param height
 * @return 		Pointer to an image instance or null on fail.
 */
QmImage *ape_editor_get_material_preview( const char *path, uint16_t width, uint16_t height );

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

QmMathVector2f *ape_grid_get_cursor_position( ApeEditorGrid *self, QmMathVector2f *dst );

/**
 * Transforms a 2D point into 3D space using the grid's transformation matrix.
 *
 * @param self 	A pointer to an ApeEditorGrid structure containing transformation information.
 * @param point A pointer to a QmMathVector2f structure representing the 2D point to be transformed.
 * @return 		A QmMathVector3f structure representing the transformed 3D point.
 */
QmMathVector3f ape_grid_transform_point( const ApeEditorGrid *self, const QmMathVector2f *point );

void ape_grid_increase_size( void );
void ape_grid_decrease_size( void );

void ape_grid_align_to_face( ApeEditorGrid *self, ApeBrushFace *face );

void ape_grid_move_forward( ApeEditorGrid *self );
void ape_grid_move_backward( ApeEditorGrid *self );

void ape_editor_on_mouse_move( ApeEditorInstance *self, const ApeViewport *viewport, int x, int y );

/////////////////////////////////////////////////////////////////////////////////////
// Brush Plotting
/////////////////////////////////////////////////////////////////////////////////////

typedef enum ApeEditorBrushType
{
	APE_EDITOR_BRUSH_TYPE_BLOCK,// traditional block-type, with 6 faces
	APE_EDITOR_BRUSH_TYPE_PLANE,// single face
} ApeEditorBrushType;

/**
 * Creates an `ApeBrush` from the polygon points stored in the `ApeEditorInstance`.
 *
 * @param self 			Pointer to the `ApeEditorInstance` which holds the polygon points and other relevant data.
 * @param materialPath	Path to the material being used for the brush.
 * @param type			The type of brush to create from the polygon.
 * @param flipFaces		Flip the faces of the brush on creation.
 * @return 				A pointer to the newly created `ApeBrush`, or `nullptr` if the brush could not be created.
 */
ApeBrush *ape_editor_mode_polygon_create( ApeEditorInstance *self, const char *materialPath, ApeEditorBrushType type, bool flipFaces );

/**
 * This function decreases the number of points in the polygon managed by the
 * given `ApeEditorInstance`. If there are no points in the polygon (`numPolygonPoints` is 0),
 * the function does nothing.
 *
 * @param self A pointer to the `ApeEditorInstance` from which the last polygon point will be removed.
 */
void ape_editor_mode_polygon_remove( ApeEditorInstance *self );

/**
 * Plots a new point for a brush. Keep in mind this is merely triggering it to occur,
 * and it will operate off whatever is the current grid point.
 *
 * @param self 	The editor state that the action is being performed within.
 * @return 			True if the point is at the same location as the origin.
 */
bool ape_editor_mode_polygon_add( ApeEditorInstance *self );

void ape_editor_mode_polygon_clear( ApeEditorInstance *instance );

/////////////////////////////////////////////////////////////////////////////////////
// Transform Mode
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Attaches all selected objects to the given node.
 *
 * @param instance	Editor instance.
 * @param parent	Node to attach selected objects to.
 */
void ape_editor_mode_transform_attach_to( const ApeEditorInstance *instance, ApeWorldNode *parent );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C_END
