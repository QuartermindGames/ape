// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_frame_viewport.h"

#include <yin/core_renderer.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>

#include <FXGLCanvas.h>
#include <FXGLVisual.h>

using namespace ss::forge;

FXGLCanvas *ViewportFrame::displayList_ = nullptr;

unsigned int ViewportFrame::cameraTagNum = 0;

FXDEFMAP( ViewportFrame )
editorViewportMap[] = {
        FXMAPFUNC( SEL_CHORE, ViewportFrame::ID_CHORE, ViewportFrame::OnChore ),
        FXMAPFUNC( SEL_MOTION, ViewportFrame::ID_CANVAS, ViewportFrame::OnMotion ),
};

FXIMPLEMENT( ViewportFrame, FXVerticalFrame, editorViewportMap, ARRAYNUMBER( editorViewportMap ) )

ViewportFrame::ViewportFrame( FXComposite *composite, FXGLVisual *visual, SSArlCameraMode viewMode )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL | LAYOUT_TOP | LAYOUT_LEFT,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )
{

	// Originally created engine camera and view here, but we'll have to defer that...
#if 0
	std::string cameraTag = "editor_camera_" + std::to_string( cameraTagNum );
	camera = ss_arl_camera_create( cameraTag.c_str(), &pl_vecOrigin3, &pl_vecOrigin3 );
	ss_arl_camera_set_view_mode( camera, viewMode );
	ss_arl_camera_set_draw_mode( camera, ( viewMode == SS_ARL_CAMERA_MODE_PERSPECTIVE ) ? SS_ARL_CAMERA_DRAW_MODE_TEXTURED : SS_ARL_CAMERA_DRAW_MODE_WIREFRAME );

	engineViewport = ss_arl_viewport_create( 0, 0, 800, 600, this );
	ss_arl_viewport_set_camera( engineViewport, camera );
#endif

	viewMode_ = viewMode;

#if 1
	toolBar_ = new FXToolBar( this, FRAME_RAISED | LAYOUT_DOCK_SAME | LAYOUT_SIDE_TOP | LAYOUT_FILL_X );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/perspective.gif" ) );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/top.gif" ) );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/left.gif" ) );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/front.gif" ) );
	new FXVerticalSeparator( toolBar_ );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/wireframe.gif" ) );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/solid.gif" ) );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/textured.gif" ) );
	new FXVerticalSeparator( toolBar_ );
	new FXTextField( toolBar_, 4, &forwardSpeedTarget_, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
	new FXTextField( toolBar_, 4, &turnSpeedTarget_, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
	new FXVerticalSeparator( toolBar_ );
	new FXButton( toolBar_, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/popout.gif" ) );
#endif

	visual_ = visual;
	if ( displayList_ == nullptr )
	{
		canvas_ = new FXGLCanvas( this, visual_, this, ID_CANVAS, LAYOUT_FILL );
		displayList_ = canvas_;
	}
	else
		canvas_ = new FXGLCanvas( this, visual_, displayList_, this, ID_CANVAS, LAYOUT_FILL );

	getApp()->addChore( this, ID_CHORE );
}

ViewportFrame::~ViewportFrame()
{
	getApp()->removeChore( this, ID_CHORE );

	ss_arl_viewport_destroy( engineViewport );

	canvas_->makeNonCurrent();
	delete canvas_;
}

void ViewportFrame::create()
{
	FXVerticalFrame::create();

	show();
	enable();

	canvas_->makeCurrent();
}

void ViewportFrame::setup_engine_viewport()
{
	std::string cameraTag = "editor_camera_" + std::to_string( cameraTagNum );
	camera = ss_arl_camera_create( cameraTag.c_str(), &pl_vecOrigin3, &pl_vecOrigin3, SS_ARL_CAMERA_MODE_PERSPECTIVE );
	ss_arl_camera_set_view_mode( camera, viewMode_ );
	ss_arl_camera_set_draw_mode( camera, ( viewMode_ == SS_ARL_CAMERA_MODE_PERSPECTIVE ) ? SS_ARL_CAMERA_DRAW_MODE_TEXTURED : SS_ARL_CAMERA_DRAW_MODE_WIREFRAME );

	engineViewport = ss_arl_viewport_create( 0, 0, 800, 600, this );
	ss_arl_viewport_set_camera( engineViewport, camera );
}

void ViewportFrame::Draw()
{
	canvas_->makeCurrent();

	int w = canvas_->getWidth();
	if ( w < 2 )
		w = 2;

	int h = canvas_->getHeight();
	if ( h < 2 )
		h = 2;

	PlgSetViewport( 0, 0, w, h );

	if ( ss_acl_is_engine_running() && engineViewport != nullptr )
	{
		ss_arl_viewport_set_camera( engineViewport, camera );
		ss_arl_viewport_set_size( engineViewport, w, h );

		ss_arl_camera_make_active( camera );

		ss_acl_render_frame( engineViewport );
	}
	else
	{
		PlgSetClearColour( PLColourRGB( 30, 30, 30 ) );
		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
	}

	if ( visual_ != nullptr && visual_->isDoubleBuffer() )
		canvas_->swapBuffers();
}

long ViewportFrame::OnChore( FXObject *, FXSelector, void * )
{
	Draw();

	getApp()->addChore( this, ID_CHORE );
	return 1;
}

long ViewportFrame::OnMotion( FXObject *, FXSelector, void *ptr )
{
	auto *event = ( FXEvent * ) ptr;
	int const x = event->win_x;
	int const y = event->win_y;

	ss_acl_input_handle_mouse_motion_event( x, y );

	return 0;
}
