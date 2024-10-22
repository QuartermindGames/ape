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
	PLVector2 position;
} GridSelectable;
static GridSelectable  gridSelectables[ APE_EDITOR_GRID_MAX_POINTS ];
static GridSelectable *activeGridSelectable;
static PLHashTable    *gridSelectablesTable;

static const float GRID_SELECTABLE_SCALE = 1.0f;

void ape_grid_setup_( ApeEditorGrid *self )
{
	self->visible = true;
	self->scale   = DEFAULT_GRID_SCALE;

#if 0

	PLMatrix4 gridRotation = PlRotateMatrix4( PL_DEG2RAD( -90.0f ), &( PLVector3 ){ 1.0f, 0.0f, 0.0f } );
	self->transform        = PlMultiplyMatrix4( PlMatrix4Identity(), &gridRotation );

#else

	self->transform = PlMatrix4Identity();

#endif

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
	float scale = ( GRID_SELECTABLE_SCALE * ( float ) self->scale ) / 8.0f;

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

void ape_grid_update_selection_( ApeEditorGrid *self )
{
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

uint ape_grid_get_size( ApeEditorGrid *self )
{
	return self->scale;
}

void ape_grid_set_visibility( ApeEditorGrid *self, bool visible )
{
	self->visible        = visible;
	activeGridSelectable = nullptr;
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

	ape_print_( "AXIS: %s, ANGLE: %f\n", PlPrintVector3( &axis, PL_VAR_F32 ), angle );

	PlRotateMatrix( angle, &axis );

	self->transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlPopMatrix();
}

void ape_grid_move_forward( ApeEditorGrid *self )
{
	PLVector3 up;
	PlExtractMatrix4Directions( &self->transform, nullptr, &up, nullptr );

	up          = PlScaleVector3F( up, self->scale / 2.0f );
	PLMatrix4 m = PlTranslateMatrix4( up );

	self->transform = PlMultiplyMatrix4( &m, &self->transform );
}

void ape_grid_move_backward( ApeEditorGrid *self )
{
	PLVector3 up;
	PlExtractMatrix4Directions( &self->transform, nullptr, &up, nullptr );

	up          = PlInverseVector3( PlScaleVector3F( up, self->scale / 2.0f ) );
	PLMatrix4 m = PlTranslateMatrix4( up );

	self->transform = PlMultiplyMatrix4( &m, &self->transform );
}

void ape_grid_draw_()
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		return;
	}

	if ( !instance->grid.visible || instance->grid.scale <= 1 )
	{
		return;
	}

	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == nullptr )
	{
		return;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &instance->grid.transform );

	int w        = APE_EDITOR_GRID_MAX_SIZE;
	int h        = APE_EDITOR_GRID_MAX_SIZE;
	int x        = -APE_EDITOR_GRID_MAX_SIZE / 2;
	int y        = -APE_EDITOR_GRID_MAX_SIZE / 2;
	int gridSize = ( instance->grid.scale / 2 );

	PLColour colour = PL_COLOURU8( 0, 0, 255, 255 );

	//TODO: cache this...
	PlgImmBegin( PLG_MESH_LINES );
	int c = 0, r = 0;
	for ( ; r < h + 1; r += gridSize )
	{
		PlgImmPushVertex( x, 0.0f, y + r );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
		PlgImmPushVertex( x + w, 0.0f, r + y );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
		for ( ; c < w + 1; c += gridSize )
		{
			PlgImmPushVertex( c + x, 0.0f, y );
			PlgImmColour( colour.r, colour.g, colour.b, colour.a );
			PlgImmPushVertex( c + x, 0.0f, y + h );
			PlgImmColour( colour.r, colour.g, colour.b, colour.a );
		}
	}
	PlgImmDraw();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	static constexpr float AXIS_SCALE = 8.0f;
	static const PLVector3 axis[]     = {
            {0.0f,       0.0f,       0.0f      },
            {AXIS_SCALE, 0.0f,       0.0f      },
            {0.0f,       0.0f,       0.0f      },
            {0.0f,       AXIS_SCALE, 0.0f      },
            {0.0f,       0.0f,       0.0f      },
            {0.0f,       0.0f,       AXIS_SCALE},
    };
	PlgDrawLines( axis, PL_ARRAY_ELEMENTS( axis ), PL_COLOUR_RED, 1.0f );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );

	PlPopMatrix();
}
