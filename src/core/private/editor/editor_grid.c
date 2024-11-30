// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "plcore/pl_hashtable.h"
#include "ape_private.h"
#include "renderer/renderer.h"
#include "editor/editor.h"

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

static constexpr uint DEFAULT_GRID_SCALE = 16;
static constexpr uint MIN_GRID_SCALE     = 1;

typedef struct GridSelectable
{
	PLColour  colour;
	PLVector2 position;
} GridSelectable;
static GridSelectable  gridSelectables[ APE_EDITOR_GRID_MAX_POINTS ];
static GridSelectable *activeGridSelectable;
static PLHashTable    *gridSelectablesTable;

static const float GRID_SELECTABLE_SCALE = 1.0f;

void ape_grid_setup_( ApeEditorGrid *self )
{
	self->visible   = true;
	self->size      = DEFAULT_GRID_SCALE;
	self->transform = PlMatrix4Identity();

	self->selectionMesh = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, 0, 0 );
	if ( self->selectionMesh == nullptr )
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

	PlgDestroyMesh( self->selectionMesh );
	self->selectionMesh = nullptr;
}

void ape_grid_toggle_command_( uint, char ** )
{
	ApeEditorInstance *state = ape_editor_get_active_instance();
	if ( state == nullptr )
	{
		return;
	}

	state->grid.visible = !state->grid.visible;
}

static void grid_batch_selection_point( const ApeEditorGrid *self, const GridSelectable *selectable )
{
	float scale = ( GRID_SELECTABLE_SCALE * ( float ) self->size ) / 8.0f;

	uint x, y, z, w;
	x = PlgPushVertex3f( self->selectionMesh, selectable->position.x + scale, 0.0f, selectable->position.y - scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	y = PlgPushVertex3f( self->selectionMesh, selectable->position.x + scale, 0.0f, selectable->position.y + scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	z = PlgPushVertex3f( self->selectionMesh, selectable->position.x - scale, 0.0f, selectable->position.y - scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	w = PlgPushVertex3f( self->selectionMesh, selectable->position.x - scale, 0.0f, selectable->position.y + scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );

	PlgPushTriangle( self->selectionMesh, x, y, z );
	PlgPushTriangle( self->selectionMesh, y, z, w );

	x = PlgPushVertex3f( self->selectionMesh, selectable->position.x + scale, scale, selectable->position.y );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	y = PlgPushVertex3f( self->selectionMesh, selectable->position.x - scale, scale, selectable->position.y );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	z = PlgPushVertex3f( self->selectionMesh, selectable->position.x + scale, -scale, selectable->position.y );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	w = PlgPushVertex3f( self->selectionMesh, selectable->position.x - scale, -scale, selectable->position.y );
	PlgColour4bv( self->selectionMesh, &selectable->colour );

	PlgPushTriangle( self->selectionMesh, x, y, z );
	PlgPushTriangle( self->selectionMesh, y, z, w );

	x = PlgPushVertex3f( self->selectionMesh, selectable->position.x, scale, selectable->position.y - scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	y = PlgPushVertex3f( self->selectionMesh, selectable->position.x, scale, selectable->position.y + scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	z = PlgPushVertex3f( self->selectionMesh, selectable->position.x, -scale, selectable->position.y - scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );
	w = PlgPushVertex3f( self->selectionMesh, selectable->position.x, -scale, selectable->position.y + scale );
	PlgColour4bv( self->selectionMesh, &selectable->colour );

	PlgPushTriangle( self->selectionMesh, x, y, z );
	PlgPushTriangle( self->selectionMesh, y, z, w );
}

/**
 * this draws what should be selectable to the selection buffer.
 */
void ape_grid_draw_selection_( ApeEditorGrid *self )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &self->transform );

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	if ( self->rebuildMesh )
	{
		PlgClearMesh( self->selectionMesh );

		int size = PlClamp( APE_EDITOR_GRID_MAX_POINTS_ROW, self->size * self->size, APE_EDITOR_GRID_MAX_POINTS );

		GridSelectable *selectable = &gridSelectables[ 0 ];
		for ( uint r = 0; r < size; r += self->size )
		{
			for ( uint c = 0; c < size; c += self->size )
			{
				selectable->position.x = ( float ) r - size / 2;
				selectable->position.y = ( float ) c - size / 2;
				grid_batch_selection_point( self, selectable );
				selectable++;
			}
		}

		PlgUploadMesh( self->selectionMesh );
		self->rebuildMesh = false;
	}

	PlgSetCullMode( PLG_CULL_NONE );

	PlgSetShaderUniformValue( PlgGetCurrentShaderProgram(), "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );
	PlgDrawMesh( self->selectionMesh );

	PlgSetCullMode( PLG_CULL_POSITIVE );

	PlPopMatrix();

	// update the active selection
	PLColour pixel;
	if ( ape_editor_get_pixel_under_cursor( &pixel ) != nullptr )
	{
		activeGridSelectable = PlLookupHashTableUserData( gridSelectablesTable, &pixel, sizeof( PLColour ) );
	}
	else
	{
		activeGridSelectable = nullptr;
	}
}

PLVector2 *ape_grid_get_cursor_position( ApeEditorGrid *self, PLVector2 *dst )
{
	if ( activeGridSelectable == nullptr )
	{
		return nullptr;
	}

	*dst = activeGridSelectable->position;
	return dst;
}

PLVector3 ape_grid_transform_point( ApeEditorGrid *self, const PLVector2 *point )
{
	return PlTransformVector3( &PL_VECTOR3( point->x, 0.0f, point->y ), &self->transform );
}

void ape_grid_increase_size( void )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->grid.size == APE_EDITOR_GRID_MAX_POINTS_ROW )
	{
		return;
	}

	instance->grid.size  = PlClamp( MIN_GRID_SCALE, instance->grid.size * 2.0f, APE_EDITOR_GRID_MAX_POINTS_ROW );
	activeGridSelectable = nullptr;

	instance->grid.rebuildMesh = true;
}

void ape_grid_decrease_size( void )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->grid.size == MIN_GRID_SCALE )
	{
		return;
	}

	instance->grid.size  = PlClamp( MIN_GRID_SCALE, instance->grid.size / 2.0f, APE_EDITOR_GRID_MAX_POINTS_ROW );
	activeGridSelectable = nullptr;

	instance->grid.rebuildMesh = true;
}

void ape_grid_align_to_face( ApeEditorGrid *self, ApeBrushFace *face )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	assert( face->numVertices > 0 );
	PlTranslateMatrix( *face->vertices[ 0 ].position );

	PLVector3 up    = { 0.0f, 1.0f, 0.0f };
	PLVector3 axis  = PlNormalizeVector3( PlVector3CrossProduct( up, face->normal ) );
	float     angle = acosf( PlVector3DotProduct( up, face->normal ) );

	//ape_print_( "AXIS: %s, ANGLE: %f\n", PlPrintVector3( &axis, PL_VAR_F32 ), angle );

	PlRotateMatrix( angle, &axis );

	self->transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlPopMatrix();
}

void ape_grid_move_forward( ApeEditorGrid *self )
{
	PLVector3 up;
	PlExtractMatrix4Directions( &self->transform, nullptr, &up, nullptr );

	up          = PlScaleVector3F( up, self->size );
	PLMatrix4 m = PlTranslateMatrix4( up );

	self->transform = PlMultiplyMatrix4( &m, &self->transform );
}

void ape_grid_move_backward( ApeEditorGrid *self )
{
	PLVector3 up;
	PlExtractMatrix4Directions( &self->transform, nullptr, &up, nullptr );

	up          = PlInverseVector3( PlScaleVector3F( up, self->size ) );
	PLMatrix4 m = PlTranslateMatrix4( up );

	self->transform = PlMultiplyMatrix4( &m, &self->transform );
}

void ape_grid_draw_( const ApeEditorGrid *self )
{
	if ( !self->visible || self->size < MIN_GRID_SCALE )
	{
		return;
	}

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &self->transform );

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	int size     = PlClamp( APE_EDITOR_GRID_MAX_POINTS_ROW, self->size * self->size, APE_EDITOR_GRID_MAX_POINTS );
	int position = -size / 2;

	PLColour colour = PL_COLOURU8( 128, 0, 128, 255 );

	//TODO: cache this...
	PlgImmBegin( PLG_MESH_LINES );
	for ( int r = 0; r < size; r += self->size )
	{
		PlgImmPushVertex( position, 0.0f, position + r );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
		PlgImmPushVertex( position + ( size - self->size ), 0.0f, r + position );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	}
	for ( int c = 0; c < size; c += self->size )
	{
		PlgImmPushVertex( c + position, 0.0f, position );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
		PlgImmPushVertex( c + position, 0.0f, position + ( size - self->size ) );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	}
	PlgImmDraw();

	PlPopMatrix();
}

void ape_grid_post_draw_( const ApeEditorGrid *self )
{
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &self->transform );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	PlgImmBegin( PLG_MESH_LINES );

	static constexpr float AXIS_SCALE = 16.0f;

	//x
	PlgImmPushVertex( 0.0f, 0.0f, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );
	PlgImmPushVertex( AXIS_SCALE, 0.0f, 0.0f );
	PlgImmColour( 255, 0, 0, 255 );
	//y
	PlgImmPushVertex( 0.0f, 0.0f, 0.0f );
	PlgImmColour( 0, 255, 0, 255 );
	PlgImmPushVertex( 0.0f, AXIS_SCALE, 0.0f );
	PlgImmColour( 0, 255, 0, 255 );
	//z
	PlgImmPushVertex( 0.0f, 0.0f, 0.0f );
	PlgImmColour( 0, 0, 255, 255 );
	PlgImmPushVertex( 0.0f, 0.0f, AXIS_SCALE );
	PlgImmColour( 0, 0, 255, 255 );

	PlgImmSetPrimitiveScale( 2.0f );
	PlgImmDraw();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );

	PlPopMatrix();
}
