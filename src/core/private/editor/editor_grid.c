// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "plcore/pl_hashtable.h"
#include "ape_private.h"
#include "camera/camera.h"
#include "editor/editor.h"
#include "renderer/material/material.h"

/////////////////////////////////////////////////////////////////////////////////////
// Grid
/////////////////////////////////////////////////////////////////////////////////////

static constexpr unsigned int DEFAULT_GRID_SCALE = 16;
static constexpr unsigned int MIN_GRID_SCALE     = 1;

void ape_grid_setup_( ApeEditorGrid *self )
{
	self->visible   = true;
	self->size      = DEFAULT_GRID_SCALE;
	self->transform = PlMatrix4Identity();
}

void ape_grid_toggle_command_( unsigned int argc, const char *const *argv )
{
	ApeEditorInstance *state = ape_editor_get_active_instance();
	if ( state == nullptr )
	{
		return;
	}

	state->grid.visible = !state->grid.visible;
}

QmMathVector2f *ape_grid_get_cursor_position( ApeEditorGrid *self, QmMathVector2f *dst )
{
	*dst = self->cursor;
	return dst;
}

static QmMathVector2f transform_world_to_grid( ApeEditorGrid *self, const QmMathVector3f *pos )
{
	PLMatrix4      transform = PlInverseMatrix4( &self->transform );
	QmMathVector3f localPos  = PlTransformVector3( pos, &transform );
	return qm_math_vector2f( localPos.x, localPos.z );
}

QmMathVector3f ape_grid_update_cursor_( ApeEditorGrid *self, int mx, int my, const ApeCamera *camera, const ApeViewport *viewport )
{
	// convert from screen to world
	QmMathVector3f pos = ape_viewport_convert_screen_to_world( viewport, ( int[] ) { mx, my }, &camera->view, &camera->proj );

	QmMathVector3f cameraPos = ape_camera_get_position( camera );

	// now setup a ray
	PLCollisionRay ray = {};
	ray.origin         = qm_math_vector3f_add( pos, cameraPos );
	ray.direction      = qm_math_vector3f_normalize( qm_math_vector3f_sub( ray.origin, cameraPos ) );

	// and the plane we're testing against (which is the grid)

	QmMathVector3f up;
	PlExtractMatrix4Directions( &self->transform, nullptr, &up, nullptr );

	PLCollisionPlane plane = {};
	plane.origin           = PlGetMatrix4Translation( &self->transform );
	plane.normal           = up;

	QmMathVector3f hitPos;
	if ( com_collision_ray_intersect_plane( &ray, &plane, &hitPos ) )
	{
		// transform it back into the grid-space, and round it
		QmMathVector2f gridPos = transform_world_to_grid( self, &hitPos );
		gridPos.x              = roundf( gridPos.x / self->size ) * self->size;
		gridPos.y              = roundf( gridPos.y / self->size ) * self->size;

		// update the cursor position
		self->cursor = gridPos;
	}

	return ape_grid_transform_point( self, &self->cursor );
}

QmMathVector3f ape_grid_transform_point( const ApeEditorGrid *self, const QmMathVector2f *point )
{
	return PlTransformVector3( &QM_MATH_VECTOR3F( point->x, 0.0f, point->y ), &self->transform );
}

void ape_grid_increase_size( void )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->grid.size == APE_EDITOR_GRID_MAX_POINTS_ROW )
	{
		return;
	}

	instance->grid.size = QM_MATH_CLAMP( MIN_GRID_SCALE, instance->grid.size * 2.0f, APE_EDITOR_GRID_MAX_POINTS_ROW );
}

void ape_grid_decrease_size( void )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->grid.size == MIN_GRID_SCALE )
	{
		return;
	}

	instance->grid.size = QM_MATH_CLAMP( MIN_GRID_SCALE, instance->grid.size / 2.0f, APE_EDITOR_GRID_MAX_POINTS_ROW );
}

void ape_grid_align_to_face( ApeEditorGrid *self, ApeBrushFace *face )
{
	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	assert( face->numVertices > 0 );
	PlTranslateMatrix( brush->vertices[ face->vertices[ 0 ].posIndex ] );

	static constexpr QmMathVector3f UP = QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f );

	QmMathVector3f axis  = qm_math_vector3f_normalize( qm_math_vector3f_cross_product( UP, face->normal ) );
	float          angle = acosf( qm_math_vector3f_dot_product( UP, face->normal ) );

	//ape_print_( "AXIS: %s, ANGLE: %f\n", PlPrintVector3( &axis, PL_VAR_F32 ), angle );

	PlRotateMatrix( angle, &axis );

	self->transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlPopMatrix();
}

void ape_grid_move_forward( ApeEditorGrid *self )
{
	QmMathVector3f up;
	PlExtractMatrix4Directions( &self->transform, nullptr, &up, nullptr );

	up          = qm_math_vector3f_scale_float( up, self->size );
	PLMatrix4 m = PlTranslateMatrix4( up );

	self->transform = PlMultiplyMatrix4( &m, &self->transform );
}

void ape_grid_move_backward( ApeEditorGrid *self )
{
	QmMathVector3f up;
	PlExtractMatrix4Directions( &self->transform, nullptr, &up, nullptr );

	up          = qm_math_vector3f_invert( qm_math_vector3f_scale_float( up, self->size ) );
	PLMatrix4 m = PlTranslateMatrix4( up );

	self->transform = PlMultiplyMatrix4( &m, &self->transform );
}

void ape_grid_draw_( const ApeEditorGrid *self )
{
	if ( !self->visible || self->size < MIN_GRID_SCALE )
	{
		return;
	}

	PlgSetBlendMode( PLG_BLEND_DEFAULT );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &self->transform );

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_GRID );

	ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT_GRID );

	//TODO: don't set these like this!!! AAaaaahhhh *melts*
	QmMathVector3f cursorPos = ape_grid_transform_point( self, &self->cursor );
	PlgSetShaderUniformValue( program->internal, "cursorPos", &cursorPos, false );
	PlgSetShaderUniformValue( program->internal, "gridScale", &self->size, false );

	int size     = QM_MATH_CLAMP( APE_EDITOR_GRID_MAX_POINTS_ROW, self->size * self->size, APE_EDITOR_GRID_MAX_POINTS );
	int position = -size / 2;

	QmMathColour4ub colour = QM_MATH_COLOUR4UB( 0, 0, 255, 255 );

	//TODO: cache this...
	PlgImmBegin( PLG_MESH_LINES );
	for ( int r = 0; r < size; r += self->size )
	{
		PlgImmPushVertex( self->cursor.x + position, 0.0f, self->cursor.y + position + r );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
		PlgImmPushVertex( self->cursor.x + position + ( size - self->size ), 0.0f, self->cursor.y + r + position );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	}
	for ( int c = 0; c < size; c += self->size )
	{
		PlgImmPushVertex( self->cursor.x + c + position, 0.0f, self->cursor.y + position );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
		PlgImmPushVertex( self->cursor.x + c + position, 0.0f, self->cursor.y + position + ( size - self->size ) );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	}
	PlgImmDraw();

	PlPopMatrix();

	PlgSetBlendMode( PLG_BLEND_DISABLE );
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
