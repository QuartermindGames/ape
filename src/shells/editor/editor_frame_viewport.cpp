// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_frame_viewport.h"

#include <yin/core_renderer.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>

#include <FXGLCanvas.h>
#include <FXGLVisual.h>

using namespace os::editor;

FXGLCanvas *ViewportFrame::displayList_ = nullptr;

FXDEFMAP( ViewportFrame )
editorViewportMap[] = {
        FXMAPFUNC( SEL_CHORE, ViewportFrame::ID_CHORE, ViewportFrame::OnChore ),
        FXMAPFUNC( SEL_MOTION, ViewportFrame::ID_CANVAS, ViewportFrame::OnMotion ),
};

FXIMPLEMENT( ViewportFrame, FXVerticalFrame, editorViewportMap, ARRAYNUMBER( editorViewportMap ) )

ViewportFrame::ViewportFrame( FXComposite *composite, FXGLVisual *visual, YNCoreCameraMode viewMode )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL | LAYOUT_TOP | LAYOUT_LEFT,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )
{
	engineViewportHandle = YnCore_Viewport_Create( 0, 0, 800, 600, this );

	//engineViewportHandle.viewMode = viewMode;
	//engineViewportHandle.drawMode = ( viewMode == YR_CAMERA_MODE_PERSPECTIVE ) ? YR_CAMERA_DRAW_MODE_TEXTURED : YR_CAMERA_DRAW_MODE_WIREFRAME;

#if 1
	toolBar_ = new FXToolBar( this, FRAME_RAISED | LAYOUT_DOCK_SAME | LAYOUT_SIDE_TOP | LAYOUT_FILL_X );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/perspective.gif" ) );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/top.gif" ) );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/left.gif" ) );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/front.gif" ) );
	new FXVerticalSeparator( toolBar_ );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/wireframe.gif" ) );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/solid.gif" ) );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/textured.gif" ) );
	new FXVerticalSeparator( toolBar_ );
	new FXTextField( toolBar_, 4, &forwardSpeedTarget_, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
	new FXTextField( toolBar_, 4, &turnSpeedTarget_, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
	new FXVerticalSeparator( toolBar_ );
	new FXButton( toolBar_, FXString::null, os::editor::LoadFXIcon( getApp(), "resources/popout.gif" ) );
#endif

	visual_ = visual;
	if ( displayList_ == nullptr )
	{
		canvas_      = new FXGLCanvas( this, visual_, this, ID_CANVAS, LAYOUT_FILL );
		displayList_ = canvas_;
	}
	else
		canvas_ = new FXGLCanvas( this, visual_, displayList_, this, ID_CANVAS, LAYOUT_FILL );

	getApp()->addChore( this, ID_CHORE );
}

ViewportFrame::~ViewportFrame()
{
	getApp()->removeChore( this, ID_CHORE );

	YnCore_Viewport_Destroy( engineViewportHandle );

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
	canvas_->makeCurrent();

	int w = canvas_->getWidth();
	if ( w < 2 )
		w = 2;

	int h = canvas_->getHeight();
	if ( h < 2 )
		h = 2;

	PlgSetViewport( 0, 0, w, h );

	if ( YnCore_IsEngineRunning() )
	{
		YnCore_Viewport_SetSize( engineViewportHandle, w, h );
		YnCore_RenderFrame( engineViewportHandle );
	}
	else
	{
		PlgSetClearColour( PLColourRGB( 30, 30, 30 ) );
		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
	}

	if ( visual_->isDoubleBuffer() )
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

	YnCore_HandleMouseMotionEvent( x, y );

	return 0;
}
