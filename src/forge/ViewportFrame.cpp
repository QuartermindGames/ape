// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "ViewportFrame.h"

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
        FXMAPFUNC( SEL_CHORE, ViewportFrame::ID_DRAW, ViewportFrame::on_chore ),
        FXMAPFUNC( SEL_MOTION, ViewportFrame::ID_CANVAS, ViewportFrame::on_motion ),
        FXMAPFUNC( SEL_RIGHTBUTTONPRESS, ViewportFrame::ID_CANVAS, ViewportFrame::on_right_click ),
};

FXIMPLEMENT( ViewportFrame, FXVerticalFrame, editorViewportMap, ARRAYNUMBER( editorViewportMap ) )

ViewportFrame::ViewportFrame( FXComposite *composite, FXGLVisual *visual, SSArlCameraMode viewMode )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL | LAYOUT_TOP | LAYOUT_LEFT,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )
{
	viewMode_ = viewMode;

#if 0
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
	{
		canvas_ = new FXGLCanvas( this, visual_, displayList_, this, ID_CANVAS, LAYOUT_FILL );
	}

	getApp()->addChore( this, ID_DRAW );
}

ViewportFrame::~ViewportFrame()
{
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

void ViewportFrame::Draw()
{
	if ( !_isActive )
		return;

	canvas_->makeCurrent();

	int w = canvas_->getWidth();
	if ( w < 2 )
		w = 2;

	int h = canvas_->getHeight();
	if ( h < 2 )
		h = 2;

	PlgSetViewport( 0, 0, w, h );

	// A lot of this is currently terrible,
	// simply because the renderer gets it's init
	// at the same time as the rest of the engine...
	// which happens AFTER the window is created (urgh)

	if ( ape_is_running() )
	{
		if ( engineViewport == nullptr )
		{
			engineViewport = ss_arl_viewport_create( 0, 0, w, h, this );
			ss_arl_viewport_set_camera( engineViewport, camera );
		}

		if ( camera == nullptr )
		{
			std::string cameraTag = "editor_camera_" + std::to_string( cameraTagNum );
			camera = ss_arl_camera_create( cameraTag.c_str(), &pl_vecOrigin3, &pl_vecOrigin3, SS_ARL_CAMERA_MODE_PERSPECTIVE );
			ss_arl_camera_set_view_mode( camera, viewMode_ );
			ss_arl_camera_set_draw_mode( camera, ( viewMode_ == SS_ARL_CAMERA_MODE_PERSPECTIVE ) ? SS_ARL_CAMERA_DRAW_MODE_TEXTURED : SS_ARL_CAMERA_DRAW_MODE_WIREFRAME );
		}

		ss_arl_viewport_set_camera( engineViewport, camera );
		ss_arl_viewport_set_size( engineViewport, w, h );

		ss_arl_camera_make_active( camera );

		ape_render_frame( engineViewport );
	}
	else
	{
		PlgSetClearColour( PLColourRGB( 30, 30, 30 ) );
		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
	}

	if ( visual_ != nullptr && visual_->isDoubleBuffer() )
		canvas_->swapBuffers();
}

long ViewportFrame::on_chore( FXObject *, FXSelector, void * )
{
	Draw();

	getApp()->addChore( this, ID_DRAW );
	return 1;
}

long ViewportFrame::on_motion( FXObject *, FXSelector, void *ptr )
{
	auto *event = ( FXEvent * ) ptr;
	int const x = event->win_x;
	int const y = event->win_y;

	ss_acl_input_handle_mouse_motion_event( x, y );

	return 0;
}

long ViewportFrame::on_right_click( FXObject *, FXSelector, void *ptr )
{
	auto event = ( FXEvent * ) ptr;
	if ( event->moved )
		return TRUE;

	// Create a pop-up menu
	auto popup = new FXMenuPane( this );

	// Add items to the menu
	( new FXMenuRadio( popup, "Perspective" ) )->setCheck( viewMode_ == SS_ARL_CAMERA_MODE_PERSPECTIVE );
	( new FXMenuRadio( popup, "Top" ) )->setCheck( viewMode_ == SS_ARL_CAMERA_MODE_TOP );
	( new FXMenuRadio( popup, "Left" ) )->setCheck( viewMode_ == SS_ARL_CAMERA_MODE_LEFT );
	( new FXMenuRadio( popup, "Front" ) )->setCheck( viewMode_ == SS_ARL_CAMERA_MODE_FRONT );
	new FXMenuSeparator( popup );
	( new FXMenuRadio( popup, "Wireframe" ) )->setCheck( drawMode_ == SS_ARL_CAMERA_DRAW_MODE_WIREFRAME );
	( new FXMenuRadio( popup, "Solid" ) )->setCheck( drawMode_ == SS_ARL_CAMERA_DRAW_MODE_SOLID );
	( new FXMenuRadio( popup, "Textured" ) )->setCheck( drawMode_ == SS_ARL_CAMERA_DRAW_MODE_TEXTURED );
	( new FXMenuRadio( popup, "Lit" ) )->setCheck( drawMode_ == SS_ARL_CAMERA_DRAW_MODE_SHADED );

	// Show the menu
	popup->create();
	popup->popup( nullptr, event->root_x, event->root_y );
	getApp()->runModalWhileShown( popup );

	delete popup;

	return TRUE;
}
