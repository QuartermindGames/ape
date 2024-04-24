// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "plcore/pl_hashtable.h"

#include "ape_private.h"

#include "client/renderer/renderer.h"
#include "editor/editor.h"

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

static const unsigned int DEFAULT_GRID_SCALE = 2;
static const unsigned int MAX_GRID_SCALE = 16;
#define GRID_SIZE     256
#define GRID_ELEMENTS ( GRID_SIZE * GRID_SIZE )

static PLMatrix4 gridTransform;

typedef struct GridSelectable
{
	PLColour colour;
	PLVector3 position;
} GridSelectable;
static GridSelectable gridSelectables[ GRID_ELEMENTS ];
static GridSelectable *activeGridSelectable;
static PLHashTable *gridSelectablesTable;

static unsigned int gridOldScale = DEFAULT_GRID_SCALE;

static const float GRID_SELECTABLE_SCALE = 0.5f;

static void grid_update_selection_points( void );

void grid_initialize_( ApeEditorState *instance )
{
	instance->gridVisible = true;
	instance->gridScale = DEFAULT_GRID_SCALE;

	gridTransform = PlMatrix4Identity();
	PLMatrix4 gridRotation = PlRotateMatrix4( PL_DEG2RAD( 90.0f ), &( PLVector3 ){ 1.0f, 0.0f, 0.0f } );
	gridTransform = PlMultiplyMatrix4( gridTransform, &gridRotation );

	gridSelectablesTable = PlCreateHashTable();

	// assign colours to each of the selection cubes
	unsigned int aaa = 1;
	for ( unsigned int i = 0; i < GRID_ELEMENTS; ++i )
	{
		gridSelectables[ i ].colour.r = aaa & 0xFF;
		gridSelectables[ i ].colour.g = ( aaa & 0xFF00 ) >> 8;
		gridSelectables[ i ].colour.b = ( aaa & 0xFF0000 ) >> 16;
		gridSelectables[ i ].colour.a = 255;
		aaa += 16;

		PlInsertHashTableNode( gridSelectablesTable, &gridSelectables[ i ].colour, sizeof( PLColour ), &gridSelectables[ i ] );
	}

	grid_update_selection_points();
}

void grid_shutdown_( void )
{
	PlDestroyHashTable( gridSelectablesTable );
	gridSelectablesTable = NULL;
}

void ape_toggle_grid_command_( unsigned int, char ** )
{
	ApeEditorState *state = ape_editor_get_active_instance();
	state->gridVisible = !state->gridVisible;
}

static void grid_update_selection_points( void )
{
	for ( unsigned int r = 0; r < GRID_SIZE; ++r )
	{
		for ( unsigned int c = 0; c < GRID_SIZE; ++c )
		{
			GridSelectable *selectable = &gridSelectables[ r * GRID_SIZE + c ];
			selectable->position.x = ( float ) r - ( ( ( float ) GRID_SIZE / 2.0f ) /* + ( GRID_SELECTABLE_SCALE / 2.0f )*/ );
			selectable->position.y = ( float ) c - ( ( ( float ) GRID_SIZE / 2.0f ) /* + ( GRID_SELECTABLE_SCALE / 2.0f )*/ );
		}
	}

	//todo: mesh should also be regenerated here
}

static void grid_batch_selection_point( const ApeCamera *camera, const GridSelectable *selectable )
{
	PLCollisionAABB bounds = ( PLCollisionAABB ){
	        .origin = PlTransformVector3( &selectable->position, &gridTransform ),
	        .maxs = ( PLVector3 ){GRID_SELECTABLE_SCALE,  GRID_SELECTABLE_SCALE,  GRID_SELECTABLE_SCALE },
	        .mins = ( PLVector3 ){-GRID_SELECTABLE_SCALE, -GRID_SELECTABLE_SCALE, -GRID_SELECTABLE_SCALE}
    };
	if ( !PlgIsBoxInsideView( camera->internal, &bounds ) )
	{
		return;
	}

	float scale = GRID_SELECTABLE_SCALE / 2.0f;

	unsigned int x, y, z, w;
	x = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y + scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	y = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y - scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	z = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y + scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	w = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y - scale, 0.0f );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );

	PlgImmPushTriangle( x, y, z );
	PlgImmPushTriangle( y, z, w );

	x = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	y = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	z = PlgImmPushVertex( selectable->position.x + scale, selectable->position.y, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	w = PlgImmPushVertex( selectable->position.x - scale, selectable->position.y, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );

	PlgImmPushTriangle( x, y, z );
	PlgImmPushTriangle( y, z, w );

	x = PlgImmPushVertex( selectable->position.x, selectable->position.y + scale, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	y = PlgImmPushVertex( selectable->position.x, selectable->position.y - scale, scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	z = PlgImmPushVertex( selectable->position.x, selectable->position.y + scale, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );
	w = PlgImmPushVertex( selectable->position.x, selectable->position.y - scale, -scale );
	PlgImmColour( selectable->colour.r, selectable->colour.g, selectable->colour.b, selectable->colour.a );

	PlgImmPushTriangle( x, y, z );
	PlgImmPushTriangle( y, z, w );
}

static void update_active_grid_selection( void )
{
	ApeViewport *selectionViewport = get_selection_viewport_();
	PLGFrameBuffer *frameBuffer = ape_render_target_get_frame_buffer( selectionViewport->renderTarget );
	if ( frameBuffer == NULL )
	{
		return;
	}

	size_t size = frameBuffer->width * frameBuffer->height * 4;
	PLColour *buf = PL_NEW_( PLColour, size );
	if ( PlgReadFrameBufferRegion( frameBuffer, 0, 0, frameBuffer->width, frameBuffer->height, size, buf ) != NULL )
	{
		int x, y;
		ape_client_input_get_mouse_position( &x, &y );

		// selection buffer is half of the source
		x /= 2;
		y /= 2;

		if ( x < frameBuffer->width && y < frameBuffer->height )
		{
			const PLColour *pixel = &buf[ ( frameBuffer->height - y - 1 ) * frameBuffer->width + x ];
			GridSelectable *selectable = PlLookupHashTableUserData( gridSelectablesTable, pixel, sizeof( PLColour ) );
			activeGridSelectable = selectable;
		}
	}
	else
	{
		ape_warning_( "Failed to read framebuffer: %s\n", PlGetError() );
	}

	PL_DELETE( buf );
}

/**
 * this draws what should be selectable to the selection buffer.
 */
static void draw_selection_grid( ApeCamera *camera )
{
	ApeEditorState *state = ape_editor_get_active_instance();

	if ( gridOldScale != state->gridScale )
	{
		grid_update_selection_points();
		gridOldScale = state->gridScale;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &gridTransform );

	PlgImmBegin( PLG_MESH_TRIANGLES );
	for ( unsigned int i = 0; i < GRID_ELEMENTS; i++ )
	{
		grid_batch_selection_point( camera, &gridSelectables[ i ] );
	}

	PlgSetCullMode( PLG_CULL_NONE );

	PlgImmDraw();

	PlgSetCullMode( PLG_CULL_POSITIVE );

	PlPopMatrix();
}

PLVector3 *ape_grid_get_cursor_position( PLVector3 *dst )
{
	if ( activeGridSelectable == NULL )
	{
		return NULL;
	}

	*dst = PlTransformVector3( &activeGridSelectable->position, &gridTransform );
	return dst;
}

void ape_grid_increase_size( void )
{
	ApeEditorState *instance = ape_editor_get_active_instance();
	if ( instance == NULL )
	{
		return;
	}

	instance->gridScale = PlClamp( DEFAULT_GRID_SCALE, ( instance->gridScale * 2 ), MAX_GRID_SCALE );
	activeGridSelectable = NULL;
}

void ape_grid_decrease_size( void )
{
	ApeEditorState *instance = ape_editor_get_active_instance();
	if ( instance == NULL )
	{
		return;
	}

	instance->gridScale = PlClamp( DEFAULT_GRID_SCALE, ( instance->gridScale / 2 ), MAX_GRID_SCALE );
	activeGridSelectable = NULL;
}

unsigned int ape_grid_get_size( void )
{
	ApeEditorState *instance = ape_editor_get_active_instance();
	if ( instance == NULL )
	{
		return 0;
	}

	return instance->gridScale;
}

void ape_grid_set_visibility( bool visible )
{
	ApeEditorState *instance = ape_editor_get_active_instance();
	if ( instance == NULL )
	{
		return;
	}

	instance->gridVisible = visible;
	activeGridSelectable = NULL;
}

void ape_grid_draw_( ApeCamera *camera )
{
	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == NULL )
	{
		return;
	}

	ApeEditorState *state = ape_editor_get_active_instance();
	if ( !ape_config_.editor || !state->gridVisible || state->gridScale <= 1 )
	{
		return;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	ApeViewport *selectionViewport = get_selection_viewport_();
	if ( state->geometryMode == APE_EDITOR_GEOMETRY_MODE_BRUSH )
	{
		unsigned int sw = viewport->width / 2;
		unsigned int sh = viewport->height / 2;
		ape_viewport_set_size( selectionViewport, sw, sh );
		ape_viewport_make_active( selectionViewport );
		ape_render_target_bind( selectionViewport->renderTarget, PLG_FRAMEBUFFER_DRAW );

		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );

		//todo: just shove this here for now for testing...
		draw_selection_grid( camera );

		update_active_grid_selection();

		ape_render_target_bind( viewport->renderTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_viewport_make_active( viewport );
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &gridTransform );

	PlgDrawGrid( -GRID_SIZE / 2, -GRID_SIZE / 2, GRID_SIZE, GRID_SIZE, state->gridScale / 2, &( PLColour ){ 0, 0, 255, 255 } );

	if ( ( state->geometryMode == APE_EDITOR_GEOMETRY_MODE_BRUSH ) && activeGridSelectable != NULL )
	{
		static const float GRID_HIGHLIGHT_SCALE = GRID_SELECTABLE_SCALE / 8.0f;

		PLCollisionAABB bounds = {
		        .origin = activeGridSelectable->position,
		        .mins = {-GRID_HIGHLIGHT_SCALE, -GRID_HIGHLIGHT_SCALE, -GRID_HIGHLIGHT_SCALE},
		        .maxs = {GRID_HIGHLIGHT_SCALE,  GRID_HIGHLIGHT_SCALE,  GRID_HIGHLIGHT_SCALE },
		};
		PlgDrawBoundingVolume( &bounds, &( PLColour ){ 255, 255, 255, 255 } );
	}

	PlPopMatrix();
}
