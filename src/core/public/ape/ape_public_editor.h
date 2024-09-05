// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "common.h"

#include "yin/core_world.h"

PL_EXTERN_C

#define APE_EDITOR_MAX_VIEWPORTS      4
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
	APE_EDITOR_GEOMETRY_MODE_SELECT,   // brush creation mode
	APE_EDITOR_GEOMETRY_MODE_FACE,     // face selection mode
	APE_EDITOR_GEOMETRY_MODE_EDGE,     // edge selection mode
	APE_EDITOR_GEOMETRY_MODE_VERTEX,   // vertex selection mode
	APE_EDITOR_GEOMETRY_MODE_TRANSFORM,// object transform mode

	APE_EDITOR_MAX_GEOMETRY_MODES
} ApeEditorGeometryMode;

typedef struct ApeEditorGrid
{
	PLMatrix4     transform;
	unsigned char visible;// unsigned char, because otherwise
	                      // can't hook it with frontend :(
	unsigned int scale;
} ApeEditorGrid;

typedef struct ApeEditorState
{
	ApeEditorGeometryMode geometryMode;
	ApeEditorGrid         grid;

	float forwardSpeed;
	float turnSpeed;

	PLLinkedList *brushPlotPoints;
} ApeEditorState;

ApeEditorState *ape_editor_instance_initialize( ApeEditorState *self );
void            ape_editor_instance_shutdown( ApeEditorState *self );
void            ape_editor_set_active_instance( ApeEditorState *self );
ApeEditorState *ape_editor_get_active_instance( void );

ApeMaterial **ape_editor_get_available_materials( unsigned int *numMaterials );

PLVector3 *ape_grid_get_cursor_position( PLVector3 *dst );

void         ape_grid_increase_size( void );
void         ape_grid_decrease_size( void );
unsigned int ape_grid_get_size( void );
void         ape_grid_set_visibility( bool visible );

/////////////////////////////////////////////////////////////////////////////////////
// Brush Plotting
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Plots a new point for a brush. Keep in mind this is merely triggering it to occur,
 * and it will operate off whatever is the current grid point.
 * @param state The editor state that the action is being performed within.
 * @return True if the point is at the same location as the origin.
 */
bool ape_editor_plot_point( ApeEditorState *state );

void ape_editor_clear_plot_points( ApeEditorState *state );

PL_EXTERN_C_END
