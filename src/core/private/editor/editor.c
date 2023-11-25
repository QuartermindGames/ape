// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Primary code for dealing with editor functionality.

#include "ape_private.h"

#include "editor.h"

#include "client/renderer/renderer.h"
#include "client/renderer/renderer_font.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static SSAclEditorGeometryMode geometryMode = EDITOR_GEOMETRYMODE_BRUSH;

static bool gridVisible = false;
static unsigned int gridScale = 1;

static void toggle_editor_command( unsigned int, char ** )
{
	ape_config_.editor = !ape_config_.editor;

	if ( ape_config_.editor )
	{
		gridVisible = true;
		gridScale = 1;
	}
	else
	{
		gridVisible = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ss_acl_increase_grid_size( void )
{
	gridScale += 2;
}

void ss_acl_decrease_grid_size( void )
{
	gridScale -= 2;

	if ( gridScale == 0 )
		gridScale = 1;
}

unsigned int ss_acl_get_grid_size( void )
{
	return gridScale;
}

void ss_acl_grid_set_visibility( bool visible )
{
}

void ss_acl_register_editor_console_variables_( void )
{
	PlRegisterConsoleCommand( "editor", "Toggle main editor functionality.", 0, toggle_editor_command );
}

void ss_acl_draw_editor_gui_( const SS_Arl_Viewport *viewport )
{
	if ( !ss_acl_is_editor_active() )
		return;

	SS_Arl_BitmapFont *font = ss_arl_get_default_bitmap_font();
	if ( font == NULL )
		return;

	SS_Arl_Camera *camera = ss_arl_camera_get_active();
	if ( camera == NULL )
		return;

	const char *label;
	ApeCameraMode cameraMode = ( camera != NULL ) ? camera->mode : SS_ARL_CAMERA_MODE_INVALID;
	switch ( cameraMode )
	{
		default:
		case SS_ARL_CAMERA_MODE_INVALID:
			label = "No camera!";
			break;
		case SS_ARL_CAMERA_MODE_PERSPECTIVE:
			label = "Perspective";
			break;
		case APE_CAMERA_MODE_FRONT:
			label = "Front";
			break;
		case APE_CAMERA_MODE_LEFT:
			label = "Left";
			break;
		case APE_CAMERA_MODE_TOP:
			label = "Top";
			break;
	}

	ss_arl_bitmap_font_draw_string( font,
	                                ( float ) ( ( viewport->width - ( font->cw * 2 ) ) - ( font->cw * strlen( label ) ) ),
	                                ( float ) ( viewport->height - ( font->ch * 2 ) ),
	                                1.0f, 1.0f, PL_COLOUR_GOLD, label, true );

	if ( cameraMode != SS_ARL_CAMERA_MODE_INVALID && cameraMode != SS_ARL_CAMERA_MODE_PERSPECTIVE )
	{
		PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

		static float z = 16.0f;
		float zoom = roundf( z ) / 2.0f;

		float x = 500.0f + sinf( zoom * 2.0f ) * 100.0f;
		float y = 200.0f + cosf( zoom * 2.0f ) * 100.0f;

		PLMatrix4 transform = PlMatrix4Identity();
		transform = PlScaleMatrix4( transform, ( PLVector3 ){ zoom, zoom, zoom } );

		// stupid matrix bollocks, blargh
		transform = PlTransposeMatrix4( &transform );
		PlgSetViewMatrix( &transform );

		int m = ( viewport->width > viewport->height ) ? viewport->width : viewport->height;
		//PlgDrawDottedGrid( -m / 2, -m / 2, m, m, gridScale / 2, &( PLColour ){ 70, 70, 70, 255 } );
		//PlgDrawDottedGrid( -m / 2, -m / 2, m, m, ( gridScale / 2 ) * 4, &( PLColour ){ 100, 100, 100, 255 } );

		switch ( cameraMode )
		{
			default:
				break;
#if 0
			case APE_CAMERA_MODE_TOP:
				transform = PlMultiplyMatrix4( transform, PlTranslateMatrix4( ( PLVector3 ){ x, -0.0f, -y } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 90.0f ), &( PLVector3 ){ 1.0f, 0.0f, 0.0f } ) );
				break;
			case APE_CAMERA_MODE_LEFT:
				transform = PlMultiplyMatrix4( transform, PlTranslateMatrix4( ( PLVector3 ){ 0.0f, -y, -x } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 90.0f ), &( PLVector3 ){ 0.0f, 1.0f, 0.0f } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 180.0f ), &( PLVector3 ){ 0.0f, 0.0f, 1.0f } ) );
				break;
			case APE_CAMERA_MODE_FRONT:
				transform = PlMultiplyMatrix4( transform, PlTranslateMatrix4( ( PLVector3 ){ -x, -y, 0.0f } ) );
				transform = PlMultiplyMatrix4( transform, PlRotateMatrix4( PL_DEG2RAD( 180.0f ), &( PLVector3 ){ 0.0f, 0.0f, 1.0f } ) );
				break;
#endif
		}

		ApeWorld *level = acl_level_get_current();
		if ( camera != NULL && level != NULL )
		{
			// stupid matrix bollocks, blargh
			transform = PlTransposeMatrix4( &transform );
			PlgSetViewMatrix( &transform );

			SS_Arl_Camera tmp;
			PL_ZERO_( tmp );
			tmp.internal = ss_arl_get_aux_camera_();
			switch ( camera->drawMode )
			{
				case SS_ARL_CAMERA_DRAW_MODE_WIREFRAME:
					arl_level_draw_wireframe( level, &tmp );
					break;
				case APE_CAMERA_DRAW_MODE_SOLID:
				case SS_ARL_CAMERA_DRAW_MODE_TEXTURED:
					arl_level_draw( level, camera, NULL, 0 );
					break;
				default:
					break;
			}

			// reset the view matrix back to it's original state
			PlgSetViewMatrix( &tmp.internal->internal.view );
		}
	}
}

bool ss_acl_is_editor_active( void )
{
	return ape_config_.editor;
}

bool apeIsEditorContextActive( const char *identifier )
{
	return false;
}
