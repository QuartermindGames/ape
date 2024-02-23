// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "forge_viewport_frame.h"
#include "forge/editors/editor_world.h"

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>

#include <FXGLCanvas.h>
#include <FXGLVisual.h>

using namespace ss::forge;

FXGLCanvas *viewport_frame::displayList_ = nullptr;
unsigned int viewport_frame::cameraTagNum = 0;

FXDEFMAP( viewport_frame )
editorViewportMap[] = {
        FXMAPFUNC( SEL_CHORE, viewport_frame::ID_DRAW, viewport_frame::on_chore ),
        FXMAPFUNC( SEL_MOTION, viewport_frame::ID_CANVAS, viewport_frame::on_motion ),
        FXMAPFUNC( SEL_MOUSEWHEEL, viewport_frame::ID_CANVAS, viewport_frame::on_zoom ),
        FXMAPFUNC( SEL_RIGHTBUTTONPRESS, viewport_frame::ID_CANVAS, viewport_frame::on_right_click ),

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

viewport_frame::viewport_frame( FXComposite *composite, FXGLVisual *visual, FXTabItem *editor, ApeCameraViewMode viewMode )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL | LAYOUT_TOP | LAYOUT_LEFT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )
{
	viewMode_ = viewMode;

	this->editor = editor;

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

	canvas_->makeCurrent();
}

void viewport_frame::Draw()
{
	if ( !_isActive )
	{
		return;
	}

	canvas_->makeCurrent();

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

	PlgSetViewport( 0, 0, w, h );

	// A lot of this is currently terrible,
	// simply because the renderer gets it's init
	// at the same time as the rest of the engine...
	// which happens AFTER the window is created (urgh)

	if ( ape_is_running() )
	{
		if ( internalViewport_ == nullptr )
		{
			internalViewport_ = ape_viewport_create( 0, 0, w, h, this );
			ape_viewport_set_camera( internalViewport_, camera );
		}

		if ( camera == nullptr )
		{
			std::string cameraTag = "editor_camera_" + std::to_string( cameraTagNum );
			camera = ape_camera_create( cameraTag.c_str(), &pl_vecOrigin3, &pl_vecOrigin3, viewMode_ );
			drawMode_ = ( viewMode_ == APE_CAMERA_MODE_PERSPECTIVE ) ? APE_CAMERA_DRAW_MODE_TEXTURED : APE_CAMERA_DRAW_MODE_WIREFRAME;
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
				ape_camera_assign_world( camera, world );
			}
		}

		ape_render_frame( internalViewport_ );
	}
	else
	{
		PlgSetClearColour( PLColourRGB( 255, 0, 0 ) );
		PlgClearBuffers( PLG_BUFFER_COLOUR | PLG_BUFFER_DEPTH );
	}

#if 0
	if ( visual_ != nullptr && visual_->isDoubleBuffer() )
	{
		canvas_->swapBuffers();
	}
#endif
}

long viewport_frame::on_change_camera_modes( FXObject *object, FX::FXSelector selector, void * )
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
		viewMode_ = viewMode;
	}
	if ( drawMode != APE_CAMERA_DRAW_MODE_INVALID )
	{
		ape_camera_set_draw_mode( camera, drawMode );
		drawMode_ = drawMode;
	}

	return TRUE;
}

long viewport_frame::on_chore( FXObject *, FXSelector, void * )
{
	Draw();

	getApp()->addChore( this, ID_DRAW );
	return 1;
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
	auto *event = ( FXEvent * ) ptr;
	int const x = event->win_x;
	int const y = event->win_y;

	//TODO: check if the game is currently active
	ape_input_handle_mouse_motion_event( x, y );

	return TRUE;
}

long viewport_frame::on_right_click( FXObject *, FXSelector, void *ptr )
{
	auto event = ( FXEvent * ) ptr;
	if ( event->moved )
	{
		return TRUE;
	}

	// Create a pop-up menu
	auto popup = new FXMenuPane( this );

	new FXMenuCommand( popup, "Create Room...", forge::load_fx_icon( getApp(), "resources/new_room.gif" ), this, ID_BUTTON_CREATE_ROOM );
	new FXMenuCommand( popup, "Create Brush...", forge::load_fx_icon( getApp(), "resources/new_brush.gif" ), this, ID_BUTTON_CREATE_BRUSH );
	new FXMenuCommand( popup, "Create Light...", forge::load_fx_icon( getApp(), "resources/new_light.gif" ), this, ID_BUTTON_CREATE_LIGHT );
	new FXMenuCommand( popup, "Create Camera...", forge::load_fx_icon( getApp(), "resources/new_camera.gif" ), this, ID_BUTTON_CREATE_CAMERA );
	new FXMenuCommand( popup, "Create Entity...", forge::load_fx_icon( getApp(), "resources/new_entity.gif" ), this, ID_BUTTON_CREATE_ENTITY );
	new FXMenuSeparator( popup );

	// Add items to the menu
	( new FXMenuRadio( popup, "Perspective", this, ID_PERSPECTIVE ) )->setCheck( viewMode_ == APE_CAMERA_MODE_PERSPECTIVE );
	( new FXMenuRadio( popup, "Top", this, ID_TOP ) )->setCheck( viewMode_ == APE_CAMERA_MODE_TOP );
	( new FXMenuRadio( popup, "Left", this, ID_LEFT ) )->setCheck( viewMode_ == APE_CAMERA_MODE_LEFT );
	( new FXMenuRadio( popup, "Front", this, ID_FRONT ) )->setCheck( viewMode_ == APE_CAMERA_MODE_FRONT );
	new FXMenuSeparator( popup );
	( new FXMenuRadio( popup, "Wireframe", this, ID_WIREFRAME ) )->setCheck( drawMode_ == APE_CAMERA_DRAW_MODE_WIREFRAME );
	( new FXMenuRadio( popup, "Solid", this, ID_SOLID ) )->setCheck( drawMode_ == APE_CAMERA_DRAW_MODE_SOLID );
	( new FXMenuRadio( popup, "Textured", this, ID_TEXTURED ) )->setCheck( drawMode_ == APE_CAMERA_DRAW_MODE_TEXTURED );
	( new FXMenuRadio( popup, "Lit", this, ID_LIT ) )->setCheck( drawMode_ == APE_CAMERA_DRAW_MODE_SHADED );
	new FXMenuSeparator( popup );
	new FXMenuCommand( popup, "Reset Camera", nullptr, this, ID_BUTTON_RESET_CAMERA );

	// Show the menu
	popup->create();
	popup->popup( nullptr, event->root_x, event->root_y );
	getApp()->runModalWhileShown( popup );

	delete popup;

	return TRUE;
}

long viewport_frame::on_key( FXObject *, FXSelector selector, void *ptr )
{
	if ( !hasFocus() )
	{
		return FALSE;
	}

	auto *event = ( FXEvent * ) ptr;
	ape_input_handle_keyboard_event( translate_key( event->code ), ( FXSELTYPE( selector ) == SEL_KEYPRESS ) );

	if ( FXSELTYPE( selector ) != SEL_KEYPRESS )
	{
		return FALSE;
	}

	static const float SPEED = 0.5f;
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
	}

	ape_camera_set_position( camera, &pos );
	ape_camera_set_angles( camera, &ang );

	return TRUE;
}

long viewport_frame::on_create( FXObject *, FXSelector selector, void * )
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

	void *data;
	ApeWorldNodeType type;
	switch ( FXSELTYPE( selector ) )
	{
		default:
			data = nullptr;
			type = APE_WORLD_NODE_TYPE_EMPTY;
			break;
		case ID_BUTTON_CREATE_ROOM:
			type = APE_WORLD_NODE_TYPE_ROOM;
			break;
		case ID_BUTTON_CREATE_BRUSH:
			type = APE_WORLD_NODE_TYPE_BRUSH;
			break;
		case ID_BUTTON_CREATE_LIGHT:
			type = APE_WORLD_NODE_TYPE_LIGHT;
			break;
		case ID_BUTTON_CREATE_CAMERA:
			type = APE_WORLD_NODE_TYPE_CAMERA;
			break;
		case ID_BUTTON_CREATE_ENTITY:
			type = APE_WORLD_NODE_TYPE_ENTITY;
			break;
	}

	ApeWorldNode *node = ape_world_node_create( world->root, "test", type, nullptr );

	worldEditor->update_tree();

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
