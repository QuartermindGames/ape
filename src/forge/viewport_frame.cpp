// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Forge viewport implementation.
// Author:  Mark E. Sowden

#include "viewport_frame.h"
#include "forge/editors/editor_world.h"
#include "ForgeMainWindow.h"

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>

#include <FXGLCanvas.h>
#include <FXGLVisual.h>

#include <X11/Xlib.h>

using namespace ss::forge;

FXGLCanvas *viewport_frame::displayList_ = nullptr;
unsigned int viewport_frame::cameraTagNum = 0;

FXDEFMAP( viewport_frame )
editorViewportMap[] = {
        FXMAPFUNC( SEL_CHORE, viewport_frame::ID_DRAW, viewport_frame::on_chore ),
        FXMAPFUNC( SEL_MOTION, viewport_frame::ID_CANVAS, viewport_frame::on_motion ),
        FXMAPFUNC( SEL_MOUSEWHEEL, viewport_frame::ID_CANVAS, viewport_frame::on_zoom ),
        FXMAPFUNC( SEL_LEFTBUTTONPRESS, viewport_frame::ID_CANVAS, viewport_frame::on_left_click ),
        FXMAPFUNC( SEL_RIGHTBUTTONPRESS, viewport_frame::ID_CANVAS, viewport_frame::on_right_click ),
        FXMAPFUNC( SEL_MIDDLEBUTTONPRESS, viewport_frame::ID_CANVAS, viewport_frame::on_middle_click ),
        FXMAPFUNC( SEL_MIDDLEBUTTONRELEASE, viewport_frame::ID_CANVAS, viewport_frame::on_middle_click ),

        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_PERSPECTIVE, viewport_frame::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_TOP, viewport_frame::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_LEFT, viewport_frame::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_FRONT, viewport_frame::on_change_camera_modes ),

        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_WIREFRAME, viewport_frame::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_SOLID, viewport_frame::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_TEXTURED, viewport_frame::on_change_camera_modes ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_LIT, viewport_frame::on_change_camera_modes ),

        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_BUTTON_CREATE_ROOM, viewport_frame::on_create ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_BUTTON_CREATE_BRUSH, viewport_frame::on_create ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_BUTTON_CREATE_LIGHT, viewport_frame::on_create ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_BUTTON_CREATE_CAMERA, viewport_frame::on_create ),
        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_BUTTON_CREATE_ENTITY, viewport_frame::on_create ),

        FXMAPFUNC( SEL_KEYPRESS, viewport_frame::ID_CANVAS, viewport_frame::on_key ),
        FXMAPFUNC( SEL_KEYRELEASE, viewport_frame::ID_CANVAS, viewport_frame::on_key ),

        FXMAPFUNC( SEL_COMMAND, viewport_frame::ID_BUTTON_RESET_CAMERA, viewport_frame::on_reset_camera ),
};

FXIMPLEMENT( viewport_frame, FXVerticalFrame, editorViewportMap, ARRAYNUMBER( editorViewportMap ) )

viewport_frame::viewport_frame( FXComposite *composite, FXGLVisual *visual, EditorTab *editor, ApeCameraViewMode viewMode )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL | LAYOUT_TOP | LAYOUT_LEFT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )
{
	viewMode_ = viewMode;
	drawMode_ = ( viewMode_ == APE_CAMERA_MODE_PERSPECTIVE ) ? APE_CAMERA_DRAW_MODE_TEXTURED : APE_CAMERA_DRAW_MODE_WIREFRAME;

	this->editor = editor;

	this->toolBar = new FXToolBar( this, FRAME_RAISED | LAYOUT_DOCK_SAME | LAYOUT_SIDE_TOP | LAYOUT_FILL_X );

	viewModeButtons[ APE_CAMERA_MODE_PERSPECTIVE ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/perspective.gif" ), nullptr, this, ID_PERSPECTIVE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_TOP ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/top.gif" ), nullptr, this, ID_TOP, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_LEFT ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/left.gif" ), nullptr, this, ID_LEFT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ APE_CAMERA_MODE_FRONT ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/front.gif" ), nullptr, this, ID_FRONT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	viewModeButtons[ viewMode_ ]->setState( true );

	new FXVerticalSeparator( this->toolBar );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_WIREFRAME ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/wireframe.gif" ), nullptr, this, ID_WIREFRAME, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_SOLID ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/solid.gif" ), nullptr, this, ID_SOLID, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_TEXTURED ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/textured.gif" ), nullptr, this, ID_TEXTURED, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ APE_CAMERA_DRAW_MODE_SHADED ] = new FXToggleButton( this->toolBar, FXString::null, FXString::null, ss::forge::load_fx_icon( getApp(), "resources/lit.gif" ), nullptr, this, ID_LIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	drawModeButtons[ drawMode_ ]->setState( true );

	if ( displayList_ == nullptr )
	{
		canvas_ = new FXGLCanvas( this, visual, this, ID_CANVAS, LAYOUT_FILL );
		displayList_ = canvas_;
	}
	else
	{
		canvas_ = new FXGLCanvas( this, visual, displayList_, this, ID_CANVAS, LAYOUT_FILL );
	}
}

viewport_frame::~viewport_frame()
{
	ape_viewport_destroy( internalViewport_ );

	canvas_->makeNonCurrent();
	delete canvas_;
}

void viewport_frame::create()
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

void viewport_frame::Draw()
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
		camera = ape_create_camera( nullptr, &pl_vecOrigin3, &pl_vecOrigin3, viewMode_, APE_CAMERA_DRAW_MODE_SHADED );
		ape_camera_set_draw_mode( camera, drawMode_ );
	}

	ape_viewport_set_camera( internalViewport_, camera );
	ape_viewport_set_size( internalViewport_, w, h );

	ape_camera_make_active( camera );

	auto *worldEditor = dynamic_cast< editor_world * >( this->editor );
	if ( worldEditor != nullptr )
	{
		ApeWorld *world = worldEditor->get_world();
		if ( world != nullptr )
		{
			ape_world_node_attach( ape_camera_get_world_node( camera ),
			                       ape_world_get_world_node( world ) );
		}
	}

	ape_render_frame( internalViewport_ );
}

long viewport_frame::on_change_camera_modes( FXObject *, FX::FXSelector selector, void * )
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

long viewport_frame::on_chore( FXObject *, FXSelector, void * )
{
	//if ( is_editor_active() )
	{
		if ( useMouseLook )
		{
			int mx, my;
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

		Draw();

		canvas_->swapBuffers();
	}

	getApp()->addChore( this, ID_DRAW );
	return TRUE;
}

long viewport_frame::on_zoom( FXObject *, FXSelector, void *ptr )
{
	auto *event = ( FXEvent * ) ptr;
	float dir = ( float ) event->code / 120;

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

long viewport_frame::on_motion( FXObject *, FXSelector, void *ptr )
{
	if ( !hasFocus() )
	{
		//	return FALSE;
	}

	auto *event = ( FXEvent * ) ptr;
	int const x = event->win_x;
	int const y = event->win_y;

	//TODO: check if the game is currently active
	ape_input_handle_mouse_motion_event( x, y );

	return TRUE;
}

long viewport_frame::on_left_click( FX::FXObject *, FX::FXSelector, void * )
{
	if ( !hasFocus() || editor == nullptr )
	{
		return FALSE;
	}

	ape_editor_plot_point( editor->get_internal() );

	return FALSE;
}

long viewport_frame::on_right_click( FXObject *, FXSelector, void *ptr )
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

	auto brushMenu = new FXMenuPane( popup );
	new FXMenuCascade( popup, "Create Brush...", forge::load_fx_icon( getApp(), "resources/new_brush.gif" ), brushMenu );
	unsigned int numBrushClasses;
	const ApeBrushClass **brushClasses = ape_get_available_brush_classes( &numBrushClasses );
	for ( unsigned int i = 0; i < numBrushClasses; ++i )
	{
		auto brushClass = brushClasses[ i ];
		auto brushCommand = new FXMenuCommand( brushMenu, brushClass->editorName, brushClass->iconSmall != nullptr ? forge::load_fx_icon( getApp(), brushClass->iconSmall ) : nullptr, this, ID_BUTTON_CREATE_BRUSH );
		brushCommand->setUserData( ( void * ) brushClass );
	}

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

long viewport_frame::on_middle_click( FXObject *, FXSelector, void * )
{
	if ( !hasFocus() )
	{
		return FALSE;
	}

	return 0;
}

long viewport_frame::on_key( FXObject *, FXSelector selector, void *ptr )
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

	ApeEditorState *instance = editor->get_internal();
	if ( instance == nullptr )
	{
		return FALSE;
	}

	static const float SPEED = 2.0f;
	PLVector3 pos = ape_camera_get_position( camera );
	PLVector3 ang = ape_camera_get_angles( camera );

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

long viewport_frame::on_create( FXObject *object, FXSelector selector, void * )
{
	auto *worldEditor = dynamic_cast< editor_world * >( this->editor );
	if ( worldEditor == nullptr )
	{
		return FALSE;
	}

	ApeWorld *world = worldEditor->get_world();
	if ( world == nullptr )
	{
		return FALSE;
	}

	ApeEditorState *instance = worldEditor->get_internal();
	assert( instance != nullptr );

	const char *name;
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

			// determine which brush class we're after...
			auto command = dynamic_cast< FXMenuCommand * >( object );
			assert( command != nullptr );
			auto brushClass = ( ApeBrushClass * ) command->getUserData();
			instance->brushClass = brushClass;
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

long viewport_frame::on_reset_camera( FXObject *, FXSelector, void * )
{
	ape_camera_set_angles( camera, &pl_vecOrigin3 );
	ape_camera_set_position( camera, &pl_vecOrigin3 );
	return TRUE;
}

int viewport_frame::translate_key( int code )
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

bool viewport_frame::is_editor_active() const
{
	if ( this->editor == nullptr )
	{
		return false;
	}

	FXTabItem *activeTab = mainWindow->get_active_tab();
	return ( activeTab == this->editor );
}
