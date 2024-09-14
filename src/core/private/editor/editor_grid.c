// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "plcore/pl_hashtable.h"
#include "ape_private.h"
#include "client/renderer/renderer.h"
#include "editor/editor.h"

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

static constexpr uint DEFAULT_GRID_SCALE = 16;
static constexpr uint MIN_GRID_SCALE     = 2;

typedef struct GridSelectable
{
	PLColour  colour;
	PLVector3 position;
} GridSelectable;
static GridSelectable  gridSelectables[ APE_EDITOR_GRID_MAX_POINTS ];
static GridSelectable *activeGridSelectable;
static PLHashTable    *gridSelectablesTable;

static const float GRID_SELECTABLE_SCALE = 1.0f;

void ape_grid_setup_( ApeEditorGrid *self )
{
	self->visible = true;
	self->scale   = DEFAULT_GRID_SCALE;

	PLMatrix4 gridRotation = PlRotateMatrix4( PL_DEG2RAD( 90.0f ), &( PLVector3 ){ 1.0f, 0.0f, 0.0f } );
	self->transform        = PlMultiplyMatrix4( PlMatrix4Identity(), &gridRotation );

	self->mesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, 0, 0 );
	if ( self->mesh == nullptr )
	{
		ape_error_( true, "Failed to create grid mesh: %s\n", PlGetError() );
	}
	self->rebuildMesh = true;

	gridSelectablesTable = PlCreateHashTable();

	// assign colours to each of the selection cubes
	uint aaa = 1;
	for ( uint i = 0; i < APE_EDITOR_GRID_MAX_POINTS; ++i )
	{
		gridSelectables[ i ].colour.r = aaa & 0xFF;
		gridSelectables[ i ].colour.g = ( aaa & 0xFF00 ) >> 8;
		gridSelectables[ i ].colour.b = ( aaa & 0xFF0000 ) >> 16;
		gridSelectables[ i ].colour.a = 255;
		aaa += 16;

		PlInsertHashTableNode( gridSelectablesTable, &gridSelectables[ i ].colour, sizeof( PLColour ), &gridSelectables[ i ] );
	}
}

void ape_grid_cleanup_( ApeEditorGrid *self )
{
	PlDestroyHashTable( gridSelectablesTable );
	gridSelectablesTable = nullptr;

	PlgDestroyMesh( self->mesh );
}

void ape_grid_toggle_command_( uint, char ** )
{
	ApeEditorInstance *state = ape_editor_get_active_instance();
	state->grid.visible      = !state->grid.visible;
}

static void grid_batch_selection_point( const ApeEditorGrid *self, const GridSelectable *selectable )
{
	float scale = ( GRID_SELECTABLE_SCALE * ( float ) self->scale ) / 8.0f;

	uint x, y, z, w;
	x = PlgPushVertex3f( self->mesh, selectable->position.x + scale, selectable->position.y + scale, 0.0f );
	PlgColour4bv( self->mesh, &selectable->colour );
	y = PlgPushVertex3f( self->mesh, selectable->position.x + scale, selectable->position.y - scale, 0.0f );
	PlgColour4bv( self->mesh, &selectable->colour );
	z = PlgPushVertex3f( self->mesh, selectable->position.x - scale, selectable->position.y + scale, 0.0f );
	PlgColour4bv( self->mesh, &selectable->colour );
	w = PlgPushVertex3f( self->mesh, selectable->position.x - scale, selectable->position.y - scale, 0.0f );
	PlgColour4bv( self->mesh, &selectable->colour );

	PlgPushTriangle( self->mesh, x, y, z );
	PlgPushTriangle( self->mesh, y, z, w );

	x = PlgPushVertex3f( self->mesh, selectable->position.x + scale, selectable->position.y, scale );
	PlgColour4bv( self->mesh, &selectable->colour );
	y = PlgPushVertex3f( self->mesh, selectable->position.x - scale, selectable->position.y, scale );
	PlgColour4bv( self->mesh, &selectable->colour );
	z = PlgPushVertex3f( self->mesh, selectable->position.x + scale, selectable->position.y, -scale );
	PlgColour4bv( self->mesh, &selectable->colour );
	w = PlgPushVertex3f( self->mesh, selectable->position.x - scale, selectable->position.y, -scale );
	PlgColour4bv( self->mesh, &selectable->colour );

	PlgPushTriangle( self->mesh, x, y, z );
	PlgPushTriangle( self->mesh, y, z, w );

	x = PlgPushVertex3f( self->mesh, selectable->position.x, selectable->position.y + scale, scale );
	PlgColour4bv( self->mesh, &selectable->colour );
	y = PlgPushVertex3f( self->mesh, selectable->position.x, selectable->position.y - scale, scale );
	PlgColour4bv( self->mesh, &selectable->colour );
	z = PlgPushVertex3f( self->mesh, selectable->position.x, selectable->position.y + scale, -scale );
	PlgColour4bv( self->mesh, &selectable->colour );
	w = PlgPushVertex3f( self->mesh, selectable->position.x, selectable->position.y - scale, -scale );
	PlgColour4bv( self->mesh, &selectable->colour );

	PlgPushTriangle( self->mesh, x, y, z );
	PlgPushTriangle( self->mesh, y, z, w );
}

static void update_active_grid_selection( void )
{
	ApeViewport    *selectionViewport = get_selection_viewport_();
	PLGFrameBuffer *frameBuffer       = ape_render_target_get_frame_buffer( selectionViewport->renderTarget );
	if ( frameBuffer == nullptr )
	{
		return;
	}

	size_t    size = frameBuffer->width * frameBuffer->height * 4;
	PLColour *buf  = PL_NEW_( PLColour, size );
	if ( PlgReadFrameBufferRegion( frameBuffer, 0, 0, frameBuffer->width, frameBuffer->height, size, buf ) != nullptr )
	{
		int x, y;
		ape_client_input_get_mouse_position( &x, &y );

		// selection buffer is half of the source
		x /= 2;
		y /= 2;

		if ( x < frameBuffer->width && y < frameBuffer->height )
		{
			const PLColour *pixel      = &buf[ ( frameBuffer->height - y - 1 ) * frameBuffer->width + x ];
			GridSelectable *selectable = PlLookupHashTableUserData( gridSelectablesTable, pixel, sizeof( PLColour ) );
			if ( selectable != nullptr )
			{
				activeGridSelectable = selectable;
			}
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
static void draw_selection_grid( ApeEditorGrid *self )
{
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &self->transform );

	if ( self->rebuildMesh )
	{
		PlgClearMesh( self->mesh );

		uint size = APE_EDITOR_GRID_MAX_SIZE / ( self->scale / 2 );
		for ( uint r = 0; r < size; ++r )
		{
			for ( uint c = 0; c < size; ++c )
			{
				GridSelectable *selectable = &gridSelectables[ r * APE_EDITOR_GRID_MAX_SIZE + c ];
				selectable->position.x     = ( float ) r * ( ( ( float ) self->scale / 2.0f ) ) - ( APE_EDITOR_GRID_MAX_SIZE / 2.0f );
				selectable->position.y     = ( float ) c * ( ( ( float ) self->scale / 2.0f ) ) - ( APE_EDITOR_GRID_MAX_SIZE / 2.0f );
				grid_batch_selection_point( self, selectable );
			}
		}

		PlgUploadMesh( self->mesh );
		self->rebuildMesh = false;
	}

	PlgSetCullMode( PLG_CULL_NONE );

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );
	PlgDrawMesh( self->mesh );

	PlgSetCullMode( PLG_CULL_POSITIVE );

	PlPopMatrix();
}

PLVector3 *ape_grid_get_cursor_position( ApeEditorGrid *self, PLVector3 *dst )
{
	if ( activeGridSelectable == nullptr )
	{
		return nullptr;
	}

	*dst = PlTransformVector3( &activeGridSelectable->position, &self->transform );
	return dst;
}

void ape_grid_increase_size( void )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->grid.scale == APE_EDITOR_GRID_MAX_SIZE )
	{
		return;
	}

	instance->grid.scale = PlClamp( MIN_GRID_SCALE, ( instance->grid.scale * 2 ), APE_EDITOR_GRID_MAX_SIZE );
	activeGridSelectable = nullptr;

	instance->grid.rebuildMesh = true;
}

void ape_grid_decrease_size( void )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->grid.scale == MIN_GRID_SCALE )
	{
		return;
	}

	instance->grid.scale = PlClamp( MIN_GRID_SCALE, ( instance->grid.scale / 2 ), APE_EDITOR_GRID_MAX_SIZE );
	activeGridSelectable = nullptr;

	instance->grid.rebuildMesh = true;
}

uint ape_grid_get_size( void )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		return 0;
	}

	return instance->grid.scale;
}

void ape_grid_set_visibility( bool visible )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		return;
	}

	instance->grid.visible = visible;
	activeGridSelectable   = nullptr;
}

void ape_grid_draw_()
{
	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == nullptr )
	{
		return;
	}

	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( !ape_config_.editor || !instance->grid.visible || instance->grid.scale <= 1 )
	{
		return;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	//#define DEBUG_GRID_SELECTION
	if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_PLOT )
	{
#if !defined( DEBUG_GRID_SELECTION )
		ApeViewport *selectionViewport = get_selection_viewport_();

		uint sw = viewport->width / 2;
		uint sh = viewport->height / 2;
		ape_viewport_set_size( selectionViewport, sw, sh );
		ape_viewport_make_active( selectionViewport );
		ape_render_target_bind( selectionViewport->renderTarget, PLG_FRAMEBUFFER_DRAW );

		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
#endif

		//todo: just shove this here for now for testing...
		draw_selection_grid( &instance->grid );

		update_active_grid_selection();

#if !defined( DEBUG_GRID_SELECTION )
		ape_render_target_bind( viewport->renderTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_viewport_make_active( viewport );
#endif
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &instance->grid.transform );

	PlgDrawGrid( -APE_EDITOR_GRID_MAX_SIZE / 2, -APE_EDITOR_GRID_MAX_SIZE / 2, APE_EDITOR_GRID_MAX_SIZE, APE_EDITOR_GRID_MAX_SIZE, instance->grid.scale / 2, &( PLColour ){ 0, 0, 255, 255 } );

	PlPopMatrix();
}
