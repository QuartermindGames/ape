// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Forge viewport implementation.
// Author:  Mark E. Sowden

#include "forge_viewport.h"
#include "forge/editors/WorldEditor.h"
#include "forge_window_main.h"

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>

#include <FXGLCanvas.h>
#include <FXGLVisual.h>

#include <X11/Xlib.h>
#include <unordered_map>

using namespace forge;

FXGLCanvas  *Viewport::displayList_ = nullptr;
unsigned int Viewport::cameraTagNum = 0;

FXDEFMAP( Viewport )
editorViewportMap[] = {
        FXMAPFUNC( SEL_CHORE, Viewport::ID_DRAW, Viewport::on_chore ),
        FXMAPFUNC( SEL_MOTION, Viewport::ID_CANVAS, Viewport::on_motion ),
        FXMAPFUNC( SEL_MOUSEWHEEL, Viewport::ID_CANVAS, Viewport::on_zoom ),
        FXMAPFUNC( SEL_LEFTBUTTONPRESS, Viewport::ID_CANVAS, Viewport::on_left_click ),
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

	viewModeButtons[ APE_CAMERA_MODE_PERSPECTIVE ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/perspective.gif" ), nullptr, this, ID_PERSPECTIVE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_TOP ]         = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/top.gif" ), nullptr, this, ID_TOP, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_LEFT ]        = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/left.gif" ), nullptr, this, ID_LEFT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_FRONT ]       = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/front.gif" ), nullptr, this, ID_FRONT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ viewMode_ ]->setState( true );

	new FXVerticalSeparator( this->toolBar );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_WIREFRAME ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/wireframe.gif" ), nullptr, this, ID_WIREFRAME, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_SOLID ]     = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/solid.gif" ), nullptr, this, ID_SOLID, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_TEXTURED ]  = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/textured.gif" ), nullptr, this, ID_TEXTURED, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_SHADED ]    = new FXToggleButton( this->toolBar, FXString::null, FXString::null, forge::load_fx_icon( getApp(), "resources/lit.gif" ), nullptr, this, ID_LIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ drawMode_ ]->setState( true );

	if ( displayList_ == nullptr )
	{
		canvas_      = new FXGLCanvas( this, visual, this, ID_CANVAS, LAYOUT_FILL );
		displayList_ = canvas_;
	}
	else
	{
		canvas_ = new FXGLCanvas( this, visual, displayList_, this, ID_CANVAS, LAYOUT_FILL );
	}
}

Viewport::~Viewport()
{
	ape_viewport_destroy( internalViewport_ );

	canvas_->makeNonCurrent();
	delete canvas_;
}

void Viewport::create()
{
	FXVerticalFrame::create();

	show();
	enable();

	if ( displayList_->getCurrentContext() == nullptr )
	{
		displayList_->makeCurrent();
	}

	getApp()->addChore( this, ID_DRAW );
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
		PlgSetViewport( 0, 0, w, h );
		PlgSetClearColour( PLColourRGB( 255, 0, 0 ) );
		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
		return;
	}

	// A lot of this is currently terrible,
	// simply because the renderer gets its init
	// at the same time as the rest of the engine...
	// which happens AFTER the window is created (urgh)

	if ( internalViewport_ == nullptr )
	{
		internalViewport_ = ape_viewport_create( 0, 0, w, h, this );
		ape_viewport_set_camera( internalViewport_, camera );
	}

	if ( camera == nullptr )
	{
		// this, again, is a gross piece of crap - it should be handled earlier!
		// lookup the first room to attach the camera to
		ApeWorldNode *parent      = nullptr;
		WorldEditor  *worldEditor = dynamic_cast< WorldEditor  *>( editor );
		if ( worldEditor != nullptr )
		{
			// fetch the first room to attach the cameras to
			PL_ITERATE_LINKED_LIST( parent, ApeWorldNode, worldEditor->get_world()->base.children )
			{
				if ( parent->type != APE_WORLD_NODE_TYPE_ROOM )
				{
					continue;
				}

				break;
			}
		}

		camera = ape_create_camera( parent, "editor_camera", &pl_vecOrigin3, &pl_vecOrigin3, viewMode_, APE_CAMERA_DRAW_MODE_SHADED );
		ape_camera_set_draw_mode( camera, drawMode_ );

		// make sure the given editor knows about the camera
		editor->set_camera( camera );
	}

	ape_viewport_set_camera( internalViewport_, camera );
	ape_viewport_set_size( internalViewport_, w, h );

	ape_camera_make_active( camera );

	ape_render_frame( internalViewport_ );
}

long Viewport::on_change_camera_modes( FXObject *, FX::FXSelector selector, void * )
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

	drawModeButtons[ drawMode_ ]->setState( false );
	viewModeButtons[ viewMode_ ]->setState( false );
	if ( viewMode != APE_CAMERA_MODE_INVALID )
	{
		ape_camera_set_view_mode( camera, viewMode );
		viewMode_ = viewMode;
		viewModeButtons[ viewMode_ ]->setState( true );
	}
	if ( drawMode != APE_CAMERA_DRAW_MODE_INVALID )
	{
		ape_camera_set_draw_mode( camera, drawMode );
		drawMode_ = drawMode;
		drawModeButtons[ drawMode_ ]->setState( true );
	}

	return TRUE;
}

long Viewport::on_chore( FXObject *, FXSelector, void * )
{
	//if ( is_editor_active() )
	{
		if ( useMouseLook )
		{
			int          mx, my;
			unsigned int tmp;
			getCursorPosition( mx, my, tmp );
			setCursorPosition( originCursorPos[ 0 ], originCursorPos[ 1 ] );

			int dx = originCursorPos[ 0 ] - mx;
			int dy = originCursorPos[ 1 ] - my;

			PLVector3 angles = ape_camera_get_angles( camera );
			angles.y += ( ( float ) dx ) / 8.0f;
			angles.x += ( ( float ) dy ) / 8.0f;

			ape_camera_set_angles( camera, &angles );
		}

		canvas_->makeCurrent();

		draw();

		canvas_->swapBuffers();
	}

	getApp()->addChore( this, ID_DRAW );
	return TRUE;
}

long Viewport::on_zoom( FXObject *, FXSelector, void *ptr )
{
	auto *event = ( FXEvent * ) ptr;
	float dir   = ( float ) event->code / 120;

	if ( viewMode_ != APE_CAMERA_MODE_PERSPECTIVE && viewMode_ != APE_CAMERA_MODE_ISOMETRIC )
	{
		internalViewport_->zoom += dir;
		printf( "zoom: %f\n", internalViewport_->zoom );
	}
	else
	{
		//TODO: check if the game is currently active
		ape_input_handle_mouse_wheel_event( 0.0f, dir );
	}

	return TRUE;
}

long Viewport::on_motion( FXObject *, FXSelector, void *ptr )
{
	if ( !hasFocus() )
	{
		//	return FALSE;
	}

	auto     *event = ( FXEvent     *) ptr;
	int const x     = event->win_x;
	int const y     = event->win_y;

	//TODO: check if the game is currently active
	ape_input_handle_mouse_motion_event( x, y );

	return TRUE;
}

long Viewport::on_left_click( FX::FXObject *, FX::FXSelector, void * )
{
	if ( !hasFocus() || editor == nullptr )
	{
		return FALSE;
	}

	ape_editor_plot_point( editor->get_internal() );

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

	return TRUE;
}

long Viewport::on_middle_click( FXObject *, FXSelector, void * )
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
		ape_input_handle_keyboard_event( translate_key( event->code ), ( FXSELTYPE( selector ) == SEL_KEYPRESS ) );
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

	static const float SPEED = 2.0f;
	PLVector3          pos   = ape_camera_get_position( camera );
	PLVector3          ang   = ape_camera_get_angles( camera );

	PLVector3 forward, left;
	PlAnglesAxes( ang, &left, nullptr, &forward );

	switch ( event->code )
	{
		default:
			break;
		case KEY_Up:
		case 'w':
		{
			pos = PlAddVector3( pos, PlScaleVector3F( forward, SPEED ) );
			break;
		}
		case KEY_Down:
		case 's':
		{
			pos = PlSubtractVector3( pos, PlScaleVector3F( forward, SPEED ) );
			break;
		}
		case KEY_Left:
		{
			ang.y += 1.5f;
			break;
		}
		case KEY_Right:
		{
			ang.y -= 1.5f;
			break;
		}
		case 'a':
		{
			pos = PlAddVector3( pos, PlScaleVector3F( left, SPEED ) );
			break;
		}
		case 'd':
		{
			pos = PlSubtractVector3( pos, PlScaleVector3F( left, SPEED ) );
			break;
		}
		case 'q':
		{
			pos.y += 0.5f;
			break;
		}
		case 'e':
		{
			pos.y -= 0.5f;
			break;
		}

		case KEY_Escape:
		{
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_SELECT )
			{
				ape_editor_clear_plot_points( instance );
			}
			break;
		}

		// grid controls
		case KEY_KP_Subtract:
		{
			ape_grid_decrease_size();
			break;
		}
		case KEY_KP_Add:
		{
			ape_grid_increase_size();
			break;
		}
	}

	ape_camera_set_position( camera, &pos );
	ape_camera_set_angles( camera, &ang );

	return TRUE;
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
