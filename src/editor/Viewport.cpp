/**********************************************************
	Huang, level editor for the Yin Game Engine.
	Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**********************************************************/

#include "qe3.h"
#include "Viewport.h"

FXGLCanvas *huang::Viewport::sharedDisplayList = nullptr;

FXDEFMAP( huang::Viewport ) ViewportMap[] = {
	FXMAPFUNC( SEL_CONFIGURE, huang::Viewport::ID_CANVAS, huang::Viewport::OnConfigure ),
	FXMAPFUNC( SEL_PAINT, huang::Viewport::ID_CANVAS, huang::Viewport::OnExpose ),
	FXMAPFUNC( SEL_CHORE, huang::Viewport::ID_CHORE,  huang::Viewport::OnChore ),

	// Input
	FXMAPFUNC( SEL_MOTION, huang::Viewport::ID_CANVAS, huang::Viewport::OnMotion ),
	FXMAPFUNC( SEL_RIGHTBUTTONPRESS, huang::Viewport::ID_CANVAS, huang::Viewport::OnInput ),
	FXMAPFUNC( SEL_RIGHTBUTTONRELEASE, huang::Viewport::ID_CANVAS, huang::Viewport::OnInput ),
	FXMAPFUNC( SEL_LEFTBUTTONPRESS, huang::Viewport::ID_CANVAS, huang::Viewport::OnInput ),

	FXMAPFUNC( SEL_COMMAND, huang::Viewport::ID_TOGGLE_VIEW, huang::Viewport::OnToggleView ),

	FXMAPFUNC( SEL_LEFTBUTTONRELEASE, huang::Viewport::ID_CANVAS, huang::Viewport::OnInput ),
	FXMAPFUNC( SEL_KEYPRESS, huang::Viewport::ID_CANVAS, huang::Viewport::OnInput ),
	FXMAPFUNC( SEL_MOUSEWHEEL, huang::Viewport::ID_CANVAS, huang::Viewport::OnInput ),

	//FXMAPFUNC( SEL_CLICKED, hg::Viewport::)
};

FXIMPLEMENT( huang::Viewport, FXVerticalFrame, ViewportMap, ARRAYNUMBER( ViewportMap ) )

huang::Viewport::Viewport( FXComposite *p, FXGLVisual *visual, ViewMode mode )
	: FXVerticalFrame( p, FRAME_NORMAL | LAYOUT_FILL_X | LAYOUT_FILL_Y | LAYOUT_TOP | LAYOUT_LEFT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ),

	myForwardSpeedTarget( camera.forwardSpeed ),
	myTurnSpeedTarget( camera.turnSpeed ) {
	currentViewMode = mode;

	memset( mouseButtonStates, 0, sizeof( bool ) * input::MAX_MOUSE_BUTTONS );

	toolBar = new FXToolBar( this, LAYOUT_DOCK_SAME | FRAME_RAISED | LAYOUT_SIDE_TOP );
	new FXToolBarGrip( toolBar, toolBar, FXToolBar::ID_TOOLBARGRIP, TOOLBARGRIP_DOUBLE );
	FXIcon *icon;
	icon = huang::util::LoadImageIcon( getApp(), "icons/perspective.gif" );
	viewModeButtons[ VIEW_MODE_PERSPECTIVE ] = new FXToggleButton( toolBar, FXString::null, FXString::null, icon, 0, this, Viewport::ID_TOGGLE_VIEW, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	icon = huang::util::LoadImageIcon( getApp(), "icons/top.gif" );
	viewModeButtons[ VIEW_MODE_TOP ] = new FXToggleButton( toolBar, FXString::null, FXString::null, icon, 0, this, Viewport::ID_TOGGLE_VIEW, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	icon = huang::util::LoadImageIcon( getApp(), "icons/left.gif" );
	viewModeButtons[ VIEW_MODE_LEFT ] = new FXToggleButton( toolBar, FXString::null, FXString::null, icon, 0, this, Viewport::ID_TOGGLE_VIEW, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	icon = huang::util::LoadImageIcon( getApp(), "icons/front.gif" );
	viewModeButtons[ VIEW_MODE_FRONT ] = new FXToggleButton( toolBar, FXString::null, FXString::null, icon, 0, this, Viewport::ID_TOGGLE_VIEW, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ mode ]->setState( true );
	new FXVerticalSeparator( toolBar );
	//new FXButton( toolBar, FXString::null, huang::util::LoadImageIcon( getApp(), "icons/cam_forward.gif" ), NULL, 0U, FRAME_NONE );
	new FXTextField( toolBar, 4, &myForwardSpeedTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
	//new FXButton( toolBar, FXString::null, huang::util::LoadImageIcon( getApp(), "icons/cam_forward.gif" ), NULL, 0U, FRAME_NONE );
	new FXTextField( toolBar, 4, &myTurnSpeedTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	glVisual = visual;
	if( sharedDisplayList == nullptr ) {
		glCanvas = new FXGLCanvas( this, visual, this, ID_CANVAS, LAYOUT_FILL );
		sharedDisplayList = glCanvas;
	} else {
		glCanvas = new FXGLCanvas( this, visual, sharedDisplayList, this, ID_CANVAS, LAYOUT_FILL );
	}

	getApp()->addChore( this, ID_CHORE );
}

huang::Viewport::~Viewport() {
	getApp()->removeChore( this, ID_CHORE );

	glCanvas->makeNonCurrent();
	delete glCanvas;

	delete toolBar;
}

void huang::Viewport::create() {
	FXVerticalFrame::create();

	show();
	enable();
}

long huang::Viewport::OnChore( FXObject *, FXSelector, void * ) {
	DrawScene();

	getApp()->addChore( this, ID_CHORE );
	return 1;
}

long huang::Viewport::OnExpose( FXObject *, FXSelector, void * ) {
	DrawScene();

	return 1;
}

long huang::Viewport::OnConfigure( FXObject *, FXSelector, void * ) {
	if( glCanvas->makeCurrent() ) {
		glViewport( 0, 0, glCanvas->getWidth(), glCanvas->getHeight() );
		glScissor( 0, 0, glCanvas->getWidth(), glCanvas->getHeight() );
		glCanvas->makeNonCurrent();

		int w = glCanvas->getWidth();
		if( w <= 0 ) {
			w = 2;
		}

		int h = glCanvas->getHeight();
		if( h <= 0 ) {
			h = 2;
		}

		switch( currentViewMode ) {
		case VIEW_MODE_PERSPECTIVE:
			camera.width = w;
			camera.height = h;
			break;
		case VIEW_MODE_TOP:
			g_qeglobals.d_xy.width = w;
			g_qeglobals.d_xy.height = h;
			break;
		}
	}

	return 1;
}

long huang::Viewport::OnMotion( FXObject *, FXSelector, void *ptr ) {
	FXEvent *ev = (FXEvent *)ptr;

	int x = ev->win_x;
	int y = -ev->win_y + glCanvas->getHeight();

#if 0
	char b[ 32 ];
	snprintf( b, sizeof( b ), "%dx%d - %dx%d\n", x, y, glCanvas->getWidth(), glCanvas->getHeight() );
	OutputDebugString( b );
#endif

	switch( currentViewMode ) {
	case VIEW_MODE_FRONT:
		XY_MouseMoved( x, y, mouseButtonStates );
		return 1;
	case VIEW_MODE_LEFT:
		//Z_MouseMoved( ev->win_x, -ev->win_y, ev->click_button );
		return 1;
	case VIEW_MODE_PERSPECTIVE:
		camera.MouseMoved( x, y, mouseButtonStates );
		return 1;
	}

	return 0;
}

long huang::Viewport::OnRightButtonPress( FXObject *, FXSelector, void * ) {
	return 0;
}

long huang::Viewport::OnRightButtonRelease( FXObject *, FXSelector, void * ) {
	return 0;
}

long huang::Viewport::OnLeftButtonPress( FXObject *, FXSelector, void * ) {
	return 0;
}

long huang::Viewport::OnLeftButtonRelease( FXObject *, FXSelector, void * ) {
	return 0;
}

static huang::input::Button TranslateButton( int fxButton ) {
	switch( fxButton ) {
	case FX::LEFTBUTTON:
		return huang::input::MOUSE_BUTTON_LEFT;
	case FX::RIGHTBUTTON:
		return huang::input::MOUSE_BUTTON_RIGHT;
	case FX::MIDDLEBUTTON:
		return huang::input::MOUSE_BUTTON_MIDDLE;
	}

	return huang::input::BUTTON_INVALID;
}

long huang::Viewport::OnInput( FXObject *, FXSelector, void *ptr ) {
	if( !isEnabled() ) {
		return 0;
	}

	FXEvent *ev = (FXEvent *)ptr;
	switch( ev->type ) {
	default:
		break;

	case SEL_KEYPRESS:
	{
		if( currentViewMode == VIEW_MODE_PERSPECTIVE && camera.HandleInput( ev->code ) ) {
			return 1;
		}

		// TODO: handle xy/z input
		break;
	}

	case SEL_LEFTBUTTONRELEASE:
	case SEL_RIGHTBUTTONRELEASE:
	case SEL_MIDDLEBUTTONRELEASE:
	{
		input::Button curButton = TranslateButton( ev->click_button );
		if( curButton == input::BUTTON_INVALID ) {
			break;
		}

		mouseButtonStates[ curButton ] = false;

		switch( currentViewMode ) {
		case VIEW_MODE_PERSPECTIVE:
			camera.MouseUp( ev->win_x, ev->win_y, mouseButtonStates );
			break;
		case VIEW_MODE_TOP:
			XY_MouseUp( ev->win_x, ev->win_y, mouseButtonStates );
			break;
		}

		break;
	}

	case SEL_LEFTBUTTONPRESS:
	case SEL_RIGHTBUTTONPRESS:
	case SEL_MIDDLEBUTTONPRESS:
	{
		input::Button curButton = TranslateButton( ev->click_button );
		if( curButton == input::BUTTON_INVALID ) {
			break;
		}

		mouseButtonStates[ curButton ] = true;

		switch( currentViewMode ) {
		case VIEW_MODE_PERSPECTIVE:
			camera.MouseDown( ev->win_x, ev->win_y, mouseButtonStates );
			break;
		case VIEW_MODE_FRONT:
		case VIEW_MODE_TOP:
			XY_MouseDown( ev->win_x, ev->win_y, mouseButtonStates );
			break;
		}

		break;
	}

	case SEL_MOUSEWHEEL:
	{
		if( ev->code > 0 ) {
			zoomScale *= 5.0f / 4.0f;
			if( zoomScale > 16.0f ) {
				zoomScale = 16.0f;
			}
		} else {
			zoomScale *= 4.0f / 5.0f;
			if( zoomScale < 0.1f ) {
				zoomScale = 0.1f;
			}
		}

		// temporary hack!!!
		if( currentViewMode == VIEW_MODE_FRONT ) {
			g_qeglobals.d_xy.scale = zoomScale;
		} else if( currentViewMode == VIEW_MODE_LEFT ) {
			z.scale = zoomScale;
		}
		return 1;
	}

	}

	return 0;
}

long huang::Viewport::OnToggleView( FXObject *object, FXSelector, void *ptr ) {
	if( !isEnabled() ) {
		return FALSE;
	}

	FXToggleButton *button = dynamic_cast<FXToggleButton *>( object );
	if( button == nullptr ) {
		return FALSE;
	}

	// Don't allow us to uncheck the same button without selecting a different one
	if( viewModeButtons[ currentViewMode ] == button ) {
		button->setState( true );
		return TRUE;
	}

	// Now figure out what mode we selected
	for( uint8_t i = 0; i < MAX_VIEW_MODES; ++i ) {
		if( viewModeButtons[ i ] == button ) {
			currentViewMode = i;
			continue;
		}

		viewModeButtons[ i ]->setState( false );
	}

	return TRUE;
}

void huang::Viewport::ResetViews() {
	camera.ResetPosition();
}

void huang::Viewport::DrawScene() {
	if( !glCanvas->makeCurrent() ) {
		Error( "MakeCurrent failed!\n" );
	}

	QE_CheckOpenGLForErrors();

	glViewport( 0, 0, glCanvas->getWidth(), glCanvas->getHeight() );
	glScissor( 0, 0, glCanvas->getWidth(), glCanvas->getHeight() );

	switch( currentViewMode ) {
	case VIEW_MODE_FRONT:
		XY_Draw( this );
		break;
	case VIEW_MODE_LEFT:
		//Z_Draw();
		break;
	case VIEW_MODE_TOP:
		XY_Draw( this );
		break;
	case VIEW_MODE_PERSPECTIVE:
		camera.draw_mode = currentDrawMode;
		camera.Draw();
		break;
	}

	glFinish();

	QE_CheckOpenGLForErrors();

	// Swap if it is double-buffered
	if( glVisual->isDoubleBuffer() ) {
		glCanvas->swapBuffers();
	}

	glCanvas->makeNonCurrent();
}
