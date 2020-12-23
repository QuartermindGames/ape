/**
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
**/

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
	: FXVerticalFrame( p, FRAME_NORMAL | LAYOUT_FILL_X | LAYOUT_FILL_Y | LAYOUT_TOP | LAYOUT_LEFT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ) {
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
	new FXVerticalSeparator( toolBar );
	viewModeButtons[ mode ]->setState( true );

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

	switch( currentViewMode ) {
	case VIEW_MODE_FRONT:
		XY_MouseMoved( ev->root_x, ev->root_y, mouseButtonStates );
		return 1;
	case VIEW_MODE_LEFT:
		//Z_MouseMoved( ev->win_x, -ev->win_y, ev->click_button );
		return 1;
	case VIEW_MODE_PERSPECTIVE:
		camera.MouseMoved( ev->win_x, ev->win_y, mouseButtonStates );
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
		return 0;
	}

	FXToggleButton *button = dynamic_cast<FXToggleButton *>( object );
	if( button == nullptr ) {
		return 0;
	}

	// Don't allow us to uncheck the same button without selecting a different one
	if( viewModeButtons[ currentViewMode ] == button ) {
		button->setState( true );
		return 0;
	}

	// Now figure out what mode we selected
	for( uint8_t i = 0; i < MAX_VIEW_MODES; ++i ) {
		if( viewModeButtons[ i ] == button ) {
			currentViewMode = i;
			continue;
		}

		viewModeButtons[ i ]->setState( false );
	}

	return 0;
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
		XY_Draw();
		break;
	case VIEW_MODE_LEFT:
		//Z_Draw();
		break;
	case VIEW_MODE_TOP:
		XY_Draw();
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
