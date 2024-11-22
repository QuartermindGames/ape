// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Vector Utility
// Author:  Mark E. Sowden

#include "../ape_private.h"

#include "editor.h"
#include "client/renderer/material/material.h"

#define APE_VECTOR_SHAPE_MAX_POINTS 32

typedef struct VectorShape
{
	PLColour  colour;
	PLVector2 points[ APE_VECTOR_SHAPE_MAX_POINTS ];
	uint      numPoints;
} VectorShape;

typedef struct VectorEditor
{
	ApeMaterial *cursorMaterial;
	int          cx, cy;

	PLVector2 points[ APE_VECTOR_SHAPE_MAX_POINTS ];
	uint      numPoints;

	VectorShape shapes[ 64 ];
} VectorEditor;

static void finalize_shape( VectorEditor *vector )
{
}

static void place_point( VectorEditor *vector )
{
	if ( vector->numPoints > 0 && ( ( ( float ) vector->cx ) == vector->points[ 0 ].x && ( ( float ) vector->cy ) == vector->points[ 0 ].y ) )
	{
		finalize_shape( vector );
		return;
	}

	if ( vector->numPoints >= APE_VECTOR_SHAPE_MAX_POINTS )
	{
		ape_warning_( "Hit maximum vector points limit (%u >= %u)!\n", vector->numPoints, APE_VECTOR_SHAPE_MAX_POINTS );
		return;
	}

	vector->points[ vector->numPoints ].x = vector->cx;
	vector->points[ vector->numPoints ].y = vector->cy;
	vector->numPoints++;

#if 0
		if ( !com_math_is_polygon_convex( vector->points, vector->numPoints ) )
		{
			vector->numPoints--;
		}
#endif
}

static void handle_action( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		return;
	}

	if ( instance->mode != APE_EDITOR_MODE_VECTOR )
	{
		return;
	}

	VectorEditor *vector = ( VectorEditor * ) instance->modeData;

	if ( strcmp( id, "editor_vector_increase_grid" ) == 0 )
	{
		ape_grid_increase_size();
	}
	else if ( strcmp( id, "editor_vector_decrease_grid" ) == 0 )
	{
		ape_grid_decrease_size();
	}
	else if ( strcmp( id, "editor_vector_place" ) == 0 )
	{
		place_point( vector );
	}
	else if ( strcmp( id, "editor_vector_undo" ) == 0 && vector->numPoints > 0 )
	{
		vector->numPoints--;
	}
}

static void initialize_vector_interface()
{
	ape_client_input_register_action( "editor_vector_increase_grid", nullptr, 0, ( ApeInputKey[] ){ 'q' }, 1, handle_action );
	ape_client_input_register_action( "editor_vector_decrease_grid", nullptr, 0, ( ApeInputKey[] ){ 'a' }, 1, handle_action );
	ape_client_input_register_action( "editor_vector_place", nullptr, 0, ( ApeInputKey[] ){ 'w' }, 1, handle_action );
	ape_client_input_register_action( "editor_vector_undo", nullptr, 0, ( ApeInputKey[] ){ 'z' }, 1, handle_action );
}

static bool setup_vector_instance( ApeEditorInstance *self )
{
	VectorEditor *vector   = PL_NEW( VectorEditor );
	vector->cursorMaterial = ape_material_cache( "materials/engine/vertex.mat.n", APE_CACHE_GROUP_EDITOR, true );

	self->grid.size = 64;

	self->modeData = vector;
	return true;
}

static void cleanup_vector_instance( ApeEditorInstance *self )
{
	VectorEditor *vector = ( VectorEditor * ) self->modeData;
	assert( vector != nullptr );

	ape_material_release( vector->cursorMaterial );

	PL_DELETE( vector );
}

static void draw_vector_overlay( ApeEditorInstance *self )
{
	VectorEditor *vector = ( VectorEditor * ) self->modeData;
	assert( vector != nullptr );

	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == nullptr )
	{
		return;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	static constexpr float CURSOR_SIZE = 8.0f;

	PlgDrawGrid( 0, 0, viewport->width, viewport->height, self->grid.size, &( PLColour ){ 0, 0, 255, 255 } );
	for ( uint i = 1; i < vector->numPoints; ++i )
	{
		PLVector3 start = PL_VECTOR3( vector->points[ i - 1 ].x, vector->points[ i - 1 ].y, 0.0f );
		PLVector3 end   = PL_VECTOR3( vector->points[ i ].x, vector->points[ i ].y, 0.0f );
		PlgDrawSimpleLine( start, end, PL_COLOUR_WHITE );
		PlgDrawRectangle( end.x - ( CURSOR_SIZE / 2.0f ), end.y - ( CURSOR_SIZE / 2.0f ), CURSOR_SIZE, CURSOR_SIZE, PL_COLOUR_YELLOW );
	}

	ape_client_input_get_mouse_position( &vector->cx, &vector->cy );

	vector->cx = PlRoundUp( vector->cx, self->grid.size );
	vector->cy = PlRoundUp( vector->cy, self->grid.size );

	ape_draw_textured_quad( vector->cursorMaterial, vector->cx - ( CURSOR_SIZE / 2.0f ), vector->cy - ( CURSOR_SIZE / 2.0f ), CURSOR_SIZE, CURSOR_SIZE, &PL_COLOUR_RED );
}

static bool save_vector( ApeEditorInstance *self, const char *path )
{
	return true;
}

const ApeEditorModeInterface ape_editorVectorModeInterface_ = {
        .initialize  = initialize_vector_interface,
        .setup       = setup_vector_instance,
        .cleanup     = cleanup_vector_instance,
        .drawOverlay = draw_vector_overlay,
        .save        = save_vector,
};
