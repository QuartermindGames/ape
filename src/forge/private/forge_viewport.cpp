// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Forge viewport implementation.
// Author:  Mark E. Sowden

#include "forge_viewport.h"
#include "forge_editor_world.h"
#include "forge_window_main.h"

#include <plgraphics/plg.h>

#include <FXGLCanvas.h>
#include <FXGLVisual.h>

#if !defined( _WIN32 )//TODO: why is this being included here!?
#	include <X11/Xlib.h>
#endif
#include <unordered_map>

using namespace forge;

FXGLCanvas *Viewport::displayList_ = nullptr;

FXDEFMAP( Viewport )
editorViewportMap[] = {
        FXMAPFUNC( SEL_TIMEOUT, Viewport::ID_DRAW, Viewport::on_timer ),

        FXMAPFUNC( SEL_MOTION, Viewport::ID_CANVAS, Viewport::on_motion ),
        FXMAPFUNC( SEL_MOUSEWHEEL, Viewport::ID_CANVAS, Viewport::on_zoom ),
        FXMAPFUNC( SEL_LEFTBUTTONPRESS, Viewport::ID_CANVAS, Viewport::on_left_click ),
        FXMAPFUNC( SEL_LEFTBUTTONRELEASE, Viewport::ID_CANVAS, Viewport::on_left_click ),
        FXMAPFUNC( SEL_RIGHTBUTTONPRESS, Viewport::ID_CANVAS, Viewport::on_right_click ),
        FXMAPFUNC( SEL_MIDDLEBUTTONPRESS, Viewport::ID_CANVAS, Viewport::on_middle_click ),
        FXMAPFUNC( SEL_MIDDLEBUTTONRELEASE, Viewport::ID_CANVAS, Viewport::on_middle_click ),

        FXMAPFUNC( SEL_COMMAND, Viewport::ID_PERSPECTIVE, Viewport::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_TOP, Viewport::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_LEFT, Viewport::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_FRONT, Viewport::on_change_camera_modes ),

        FXMAPFUNC( SEL_COMMAND, Viewport::ID_WIREFRAME, Viewport::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_SOLID, Viewport::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_TEXTURED, Viewport::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_LIT, Viewport::on_change_camera_modes ),

        FXMAPFUNC( SEL_COMMAND, Viewport::ID_BUTTON_CREATE_ROOM, Viewport::on_create ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_BUTTON_CREATE_BRUSH, Viewport::on_create ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_BUTTON_CREATE_LIGHT, Viewport::on_create ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_BUTTON_CREATE_CAMERA, Viewport::on_create ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_BUTTON_CREATE_ENTITY, Viewport::on_create ),

        FXMAPFUNC( SEL_KEYPRESS, Viewport::ID_CANVAS, Viewport::on_key ),
        FXMAPFUNC( SEL_KEYRELEASE, Viewport::ID_CANVAS, Viewport::on_key ),

        FXMAPFUNC( SEL_COMMAND, Viewport::ID_BUTTON_SCREENSHOT, Viewport::on_screenshot ),
        FXMAPFUNC( SEL_COMMAND, Viewport::ID_BUTTON_RESET_CAMERA, Viewport::on_reset_camera ),
};

FXIMPLEMENT( Viewport, FXVerticalFrame, editorViewportMap, ARRAYNUMBER( editorViewportMap ) )

Viewport::Viewport( FXComposite *composite, FXGLVisual *visual, EditorTab *editor, ApeCameraViewMode viewMode )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL | LAYOUT_TOP | LAYOUT_LEFT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )
{
	viewMode_ = viewMode;
	drawMode_ = ( viewMode_ == APE_CAMERA_MODE_PERSPECTIVE ) ? APE_CAMERA_DRAW_MODE_TEXTURED : APE_CAMERA_DRAW_MODE_WIREFRAME;

	this->editor = editor;

	this->toolBar = new FXToolBar( this, FRAME_RAISED | LAYOUT_DOCK_SAME | LAYOUT_SIDE_TOP | LAYOUT_FILL_X );

	viewModeButtons[ APE_CAMERA_MODE_PERSPECTIVE ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/perspective.gif" ), nullptr, this, ID_PERSPECTIVE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_TOP ]         = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/top.gif" ), nullptr, this, ID_TOP, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_LEFT ]        = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/left.gif" ), nullptr, this, ID_LEFT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_FRONT ]       = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/front.gif" ), nullptr, this, ID_FRONT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ viewMode_ ]->setState( true );

	new FXVerticalSeparator( this->toolBar );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_WIREFRAME ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/wireframe.gif" ), nullptr, this, ID_WIREFRAME, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_SOLID ]     = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/solid.gif" ), nullptr, this, ID_SOLID, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_TEXTURED ]  = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/textured.gif" ), nullptr, this, ID_TEXTURED, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_SHADED ]    = new FXToggleButton( this->toolBar, FXString::null, FXString::null, load_fx_icon( getApp(), "resources/lit.gif" ), nullptr, this, ID_LIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ drawMode_ ]->setState( true );

	new FXVerticalSeparator( this->toolBar );
	new FXButton( this->toolBar, FXString::null, load_fx_icon( getApp(), "resources/screenshot.gif" ), this, ID_BUTTON_SCREENSHOT );

	new FXVerticalSeparator( this->toolBar );
	new FXLabel( this->toolBar, FXString::null, load_fx_icon( getApp(), "resources/cam_static.gif" ) );
	cameraSpeedSlider = new FXSlider( this->toolBar, nullptr, 0, LAYOUT_FIX_WIDTH | SLIDER_HORIZONTAL | SLIDER_ARROW_RIGHT | SLIDER_TICKS_BOTTOM );
	cameraSpeedSlider->setWidth( 64 );
	cameraSpeedSlider->setRange( 1, 8 );
	cameraSpeedSlider->setIncrement( 1 );
	cameraSpeedSlider->setValue( 4 );
	new FXLabel( this->toolBar, FXString::null, load_fx_icon( getApp(), "resources/cam_speed.gif" ) );

	canvas_ = new FXGLCanvas( this, visual, displayList_, this, ID_CANVAS, LAYOUT_FILL );
	if ( displayList_ == nullptr )
	{
		displayList_ = canvas_;
	}

	QmMathVector3f position = qm_math_vector3f( 0.0f, 80.0f, 0.0f );

	camera = ape_create_camera( nullptr, "editor_camera", &position, &pl_vecOrigin3, viewMode_, APE_CAMERA_DRAW_MODE_SHADED );
	ape_camera_set_draw_mode( camera, drawMode_ );

	APE_WORLD_NODE( camera )->flags |= APE_WORLD_NODE_FLAG_DISCARD;

	if ( editor != nullptr )
	{
		// make sure the given editor knows about the camera
		editor->set_camera( camera );
	}
}

Viewport::~Viewport()
{
	getApp()->removeTimeout( this, ID_DRAW );

	ape_viewport_destroy( internalViewport_ );

	canvas_->makeNonCurrent();
	canvas_->detach();

	delete canvas_;
}

void Viewport::create()
{
	FXVerticalFrame::create();

	show();
	enable();

	if ( FXGLCanvas::getCurrentContext() == nullptr )
	{
		displayList_->makeCurrent();
	}

	getApp()->addTimeout( this, ID_DRAW, APE_DEFAULT_TICK_RATE );
}

void Viewport::draw()
{
	if ( !is_editor_active() )
	{
		return;
	}

	int w = canvas_->getWidth();
	if ( w < 2 )
	{
		w = 2;
	}

	int h = canvas_->getHeight();
	if ( h < 2 )
	{
		h = 2;
	}

	if ( !ape_is_running() )
	{
		qm_gfx_set_viewport( 0, 0, w, h );
		PlgSetClearColour( QM_MATH_COLOUR4UB_RGB( 255, 0, 0 ) );
		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
		return;
	}

	if ( internalViewport_ == nullptr )
	{
		internalViewport_ = ape_viewport_create( 0, 0, w, h, this, true );
		ape_viewport_set_camera( internalViewport_, camera );
	}

	ape_viewport_set_camera( internalViewport_, camera );
	ape_viewport_set_size( internalViewport_, w, h );

	ape_camera_make_active( camera );

	ape_render_frame( internalViewport_ );
}

long Viewport::on_change_camera_modes( FXObject *, FXSelector selector, void * )
{
	ApeCameraViewMode viewMode = APE_CAMERA_MODE_INVALID;
	ApeCameraDrawMode drawMode = APE_CAMERA_DRAW_MODE_INVALID;

	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_PERSPECTIVE:
			viewMode = APE_CAMERA_MODE_PERSPECTIVE;
			break;
		case ID_TOP:
			viewMode = APE_CAMERA_MODE_TOP;
			break;
		case ID_LEFT:
			viewMode = APE_CAMERA_MODE_LEFT;
			break;
		case ID_FRONT:
			viewMode = APE_CAMERA_MODE_FRONT;
			break;
		case ID_WIREFRAME:
			drawMode = APE_CAMERA_DRAW_MODE_WIREFRAME;
			break;
		case ID_SOLID:
			drawMode = APE_CAMERA_DRAW_MODE_SOLID;
			break;
		case ID_TEXTURED:
			drawMode = APE_CAMERA_DRAW_MODE_TEXTURED;
			break;
		case ID_LIT:
			drawMode = APE_CAMERA_DRAW_MODE_SHADED;
			break;
	}

	if ( viewMode != APE_CAMERA_MODE_INVALID )
	{
		ape_camera_set_view_mode( camera, viewMode );
		viewModeButtons[ viewMode_ ]->setState( false );
		viewMode_ = viewMode;
		viewModeButtons[ viewMode_ ]->setState( true );
	}
	if ( drawMode != APE_CAMERA_DRAW_MODE_INVALID )
	{
		ape_camera_set_draw_mode( camera, drawMode );
		drawModeButtons[ drawMode_ ]->setState( false );
		drawMode_ = drawMode;
		drawModeButtons[ drawMode_ ]->setState( true );
	}

	return TRUE;
}

long Viewport::on_timer( FXObject *, FXSelector, void * )
{
	if ( canvas_->makeCurrent() )
	{
		if ( useMouseLook )
		{
			int          mx, my;
			unsigned int tmp;
			getCursorPosition( mx, my, tmp );

			int dx = originCursorPos[ 0 ] - mx;
			int dy = originCursorPos[ 1 ] - my;

			QmMathVector3f angles = ape_camera_get_angles( camera );
			angles.y += ( float ) dx / ( this->width / 10.0f );
			angles.x += ( float ) dy / ( this->height / 10.0f );

			ape_camera_set_angles( camera, &angles );
		}

		draw();

		canvas_->swapBuffers();
	}

	getApp()->addTimeout( this, ID_DRAW, APE_DEFAULT_TICK_RATE );
	return TRUE;
}

long Viewport::on_zoom( FXObject *, FXSelector, void *ptr )
{
	auto *event = ( FXEvent * ) ptr;
	float dir   = ( float ) event->code;

	if ( viewMode_ != APE_CAMERA_MODE_PERSPECTIVE && viewMode_ != APE_CAMERA_MODE_ISOMETRIC )
	{
		internalViewport_->zoom += dir / 120.0f;
	}
	else
	{
		QmMathVector3f pos     = ape_camera_get_position( camera );
		QmMathVector3f forward = ape_camera_get_forward( camera );

		dir /= 50.0f;
		if ( dir )
		{
			pos = qm_math_vector3f_sub( pos, qm_math_vector3f_scale_float( forward, dir ) );
		}
		else
		{
			pos = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( forward, dir ) );
		}

		ape_camera_set_position( camera, &pos );
	}

	return TRUE;
}

long Viewport::on_motion( FXObject *, FXSelector, void *ptr )
{
	if ( !hasFocus() )
	{
		return FALSE;
	}

	auto     *event = ( FXEvent * ) ptr;
	int const x     = event->win_x;
	int const y     = event->win_y;

	//TODO: check if the game is currently active
	ape_input_handle_mouse_motion_event( x, y );

	return TRUE;
}

long Viewport::on_left_click( FXObject *, FXSelector selector, void * )
{
	if ( !hasFocus() )
	{
		return FALSE;
	}

	if ( mainWindow->is_game_running() )
	{
		//ape_input_handle_mouse_button_event( APE_INPUT_MOUSE_BUTTON_LEFT, ( FXSELTYPE( selector ) == SEL_KEYPRESS ) );
	}

	return FALSE;
}

long Viewport::on_right_click( FXObject *, FXSelector, void *ptr )
{
	if ( !hasFocus() )
	{
		return FALSE;
	}

	// Can't interact if mouse look is active!
	if ( useMouseLook )
	{
		return FALSE;
	}

	auto event = ( FXEvent * ) ptr;
	if ( event->moved )
	{
		return TRUE;
	}

#if 0
	// Create a pop-up menu
	auto popup = new FXMenuPane( this );
	new FXMenuCommand( popup, "Create Brush...", forge::load_fx_icon( getApp(), "resources/new_brush.gif" ), this, ID_BUTTON_CREATE_BRUSH );
	new FXMenuCommand( popup, "Create Room...", forge::load_fx_icon( getApp(), "resources/new_room.gif" ), this, ID_BUTTON_CREATE_ROOM );
	new FXMenuCommand( popup, "Create Light...", forge::load_fx_icon( getApp(), "resources/new_light.gif" ), this, ID_BUTTON_CREATE_LIGHT );
	new FXMenuCommand( popup, "Create Camera...", forge::load_fx_icon( getApp(), "resources/new_camera.gif" ), this, ID_BUTTON_CREATE_CAMERA );
	new FXMenuCommand( popup, "Create Entity...", forge::load_fx_icon( getApp(), "resources/new_entity.gif" ), this, ID_BUTTON_CREATE_ENTITY );

	new FXMenuSeparator( popup );
	new FXMenuCommand( popup, "Reset Camera", nullptr, this, ID_BUTTON_RESET_CAMERA );

	// Show the menu
	popup->create();
	popup->popup( nullptr, event->root_x, event->root_y );
	getApp()->runModalWhileShown( popup );

	delete popup;
#endif

	return FALSE;
}

long Viewport::on_middle_click( FXObject *, FXSelector selector, void * )
{
	if ( !hasFocus() )
	{
		return FALSE;
	}

	return 0;
}

long Viewport::on_key( FXObject *, FXSelector selector, void *ptr )
{
	if ( !hasFocus() )
	{
		return FALSE;
	}

	auto *event = ( FXEvent * ) ptr;
	if ( mainWindow->is_game_running() )
	{
		ape_input_handle_keyboard_event( translate_key( event->code ), FXSELTYPE( selector ) == SEL_KEYPRESS );
		return TRUE;
	}

	if ( FXSELTYPE( selector ) != SEL_KEYPRESS )
	{
		return FALSE;
	}

	if ( event->code == 'f' )
	{
		useMouseLook = !useMouseLook;
		if ( useMouseLook )
		{
			FXuint tmp;
			getCursorPosition( originCursorPos[ 0 ], originCursorPos[ 1 ], tmp );
		}
		return TRUE;
	}

	ApeEditorInstance *instance = editor->get_internal();
	if ( instance == nullptr )
	{
		return FALSE;
	}

	float          speed = ( float ) cameraSpeedSlider->getValue();
	QmMathVector3f pos   = ape_camera_get_position( camera );
	QmMathVector3f ang   = ape_camera_get_angles( camera );

	bool handled = true;
	switch ( event->code )
	{
		default:
			handled = false;
			break;

		case KEY_Up:
		{
			if ( event->state & SHIFTMASK )
			{
				return false;
			}
			ang.x += speed;
			break;
		}
		case KEY_Down:
		{
			if ( event->state & SHIFTMASK )
			{
				return false;
			}
			ang.x -= speed;
			break;
		}
		case KEY_Left:
		{
			if ( event->state & SHIFTMASK )
			{
				return false;
			}
			ang.y += 1.5f;
			break;
		}
		case KEY_Right:
		{
			if ( event->state & SHIFTMASK )
			{
				return false;
			}
			ang.y -= 1.5f;
			break;
		}

		case 'w':
		{
			pos = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( ape_camera_get_forward( camera ), -speed ) );
			break;
		}
		case 's':
		{
			pos = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( ape_camera_get_forward( camera ), speed ) );
			break;
		}
		case 'a':
		{
			QmMathVector3f left;
			PlAnglesAxes( ang, &left, nullptr, nullptr );
			pos = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( left, speed ) );
			break;
		}
		case 'd':
		{
			QmMathVector3f left;
			PlAnglesAxes( ang, &left, nullptr, nullptr );
			pos = qm_math_vector3f_sub( pos, qm_math_vector3f_scale_float( left, speed ) );
			break;
		}
		case 'q':
		{
			pos.y += speed;
			break;
		}
		case 'e':
		{
			pos.y -= speed;
			break;
		}
	}

	ape_camera_set_position( camera, &pos );
	ape_camera_set_angles( camera, &ang );

	return handled;
}

long Viewport::on_create( FXObject *object, FXSelector selector, void * )
{
	auto *worldEditor = dynamic_cast< WorldEditor * >( this->editor );
	if ( worldEditor == nullptr )
	{
		return FALSE;
	}

	ApeWorld *world = worldEditor->get_world();
	if ( world == nullptr )
	{
		return FALSE;
	}

	ApeEditorInstance *instance = worldEditor->get_internal();
	assert( instance != nullptr );

	const char      *name;
	ApeWorldNodeType type;
	switch ( FXSELID( selector ) )
	{
		default:
		{
			name = "empty";
			type = APE_WORLD_NODE_TYPE_EMPTY;
			break;
		}
		case ID_BUTTON_CREATE_ROOM:
		{
			name = "room";
			type = APE_WORLD_NODE_TYPE_ROOM;
			break;
		}
		case ID_BUTTON_CREATE_BRUSH:
		{
			name = "brush";
			type = APE_WORLD_NODE_TYPE_BRUSH;
			break;
		}
		case ID_BUTTON_CREATE_LIGHT:
		{
			name = "light";
			type = APE_WORLD_NODE_TYPE_LIGHT;
			break;
		}
		case ID_BUTTON_CREATE_CAMERA:
		{
			name = "camera";
			type = APE_WORLD_NODE_TYPE_CAMERA;
			break;
		}
		case ID_BUTTON_CREATE_ENTITY:
		{
			name = "entity";
			type = APE_WORLD_NODE_TYPE_ENTITY;
			break;
		}
	}

	worldEditor->create_new_object( name, type );

	return TRUE;
}

long Viewport::on_screenshot( FXObject *, FXSelector, void * )
{
	PlParseConsoleString( "screenshot" );

	return TRUE;
}

long Viewport::on_reset_camera( FXObject *, FXSelector, void * )
{
	ape_camera_set_angles( camera, &pl_vecOrigin3 );
	ape_camera_set_position( camera, &pl_vecOrigin3 );
	return TRUE;
}

int Viewport::translate_key( int code )
{
	switch ( code )
	{
		default:
			return code;
		case KEY_Up:
			return APE_INPUT_KEY_UP;
		case KEY_Down:
			return APE_INPUT_KEY_DOWN;
		case KEY_Left:
			return APE_INPUT_KEY_LEFT;
		case KEY_Right:
			return APE_INPUT_KEY_RIGHT;
	}
}

bool Viewport::is_editor_active() const
{
	if ( this->editor == nullptr )
	{
		return false;
	}

	FXTabItem *activeTab = mainWindow->get_active_tab();
	return ( activeTab == this->editor );
}
