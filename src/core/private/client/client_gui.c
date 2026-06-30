// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "ape_private.h"
#include "client_gui.h"

#include "camera/camera.h"
#include "yin/core_interfaces.h"

#include "editor/editor.h"

#include "game/game_public.h"

#include "renderer/renderer.h"
#include "renderer/material/material.h"
#include "renderer/post/post.h"

static ApeMaterial *baseGuiMat;

static ApeCamera *auxCamera;

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_gui_( void )
{
	auxCamera = ape_create_camera( nullptr, nullptr, &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO, APE_CAMERA_MODE_ORTHOGRAPHIC, APE_CAMERA_DRAW_MODE_SHADED );
	if ( auxCamera == nullptr )
	{
		ape_console_error_( true, "Failed to create auxiliary camera!\n" );
	}

	ape_gui_initialize_();
}

void ape_shutdown_gui_( void )
{
	ape_gui_shutdown_();

	ape_material_release_reference( baseGuiMat );
}

void ape_setup_2d_viewport_( int w, int h )
{
	qm_gfx_set_viewport( 0, 0, w, h );
	ape_camera_setup( auxCamera );
}

void ape_get_2d_viewport_size_( int *width, int *height )
{
	qm_gfx_get_viewport( nullptr, nullptr, width, height );
}

void ape_renderer_draw_menu( ApeViewport *viewport )
{
	if ( viewport == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	ape_viewport_make_active( viewport );
	ape_setup_2d_viewport_( viewport->width, viewport->height );

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_DISABLE );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	ape_gui_draw_( viewport );

	PlPopMatrix();

	PlgSetDepthBufferMode( PLG_DEPTHBUFFER_ENABLE );

	COM_PROFILE_FUNCTION_END();
}

void ape_console_update_notifications_( double delta );// client_console.c

void ape_tick_gui_( const double delta )
{
	COM_PROFILE_FUNCTION_START();

	ape_console_update_notifications_( delta );

	COM_PROFILE_FUNCTION_END();
}
