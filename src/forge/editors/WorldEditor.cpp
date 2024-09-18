// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: World editor tab
// Author:  Mark E. Sowden

#include <memory>
#include <unordered_map>

#include "WorldEditor.h"

#include "forge/forge_viewport.h"

FXDEFMAP( forge::WorldEditor )
worldEditorMap[] = {
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_POLY_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_FACE_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_EDGE_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_VERTEX_MODE, forge::WorldEditor::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_TRANSFORM_MODE, forge::WorldEditor::on_change_geometry_mode ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_SELECT, forge::WorldEditor::on_room_select ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_ROOM_NEW, forge::WorldEditor::on_new_room ),

        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_GRID_UP, forge::WorldEditor::on_shift_grid ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_GRID_DOWN, forge::WorldEditor::on_shift_grid ),
        FXMAPFUNC( SEL_COMMAND, forge::WorldEditor::ID_GRID_ALIGN, forge::WorldEditor::on_shift_grid ),
};
FXIMPLEMENT( forge::WorldEditor, EditorTab, worldEditorMap, ARRAYNUMBER( worldEditorMap ) )

forge::WorldEditor::WorldEditor( FXTabBook *owner, const FXString &worldName, ApeWorld *world )
    : EditorTab( owner, "World Editor", forge_cachedIcons[ FORGE_ICON_TYPE_WORLD ], APE_EDITOR_MODE_WORLD ),
      _gridSizeTarget( this->instance.grid.scale ),
      _gridHideTarget( this->instance.grid.visible )
{
	auto *middleFrame = new FXVerticalFrame( owner, FRAME_RAISED | LAYOUT_FILL );

	auto *toolbar = new FXToolBar( middleFrame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/save.gif" ) );
	new FXVerticalSeparator( toolbar );

	// room selection box
	roomSelectBox = new FXComboBox( toolbar, 16, this, ID_ROOM_SELECT, COMBOBOX_REPLACE | FRAME_SUNKEN | FRAME_THICK | LAYOUT_CENTER_Y | LAYOUT_FILL_COLUMN | LAYOUT_MIN_WIDTH, 0, 0, 400 );
	roomSelectBox->setNumVisible( 8 );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/new_room.gif" ), this, ID_ROOM_NEW );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/room_edit.gif" ), this, ID_ROOM_EDIT );
	new FXVerticalSeparator( toolbar );

	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_PLOT ]      = new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/edit_polygon.gif" ), nullptr, this, ID_POLY_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_VERTEX ]    = new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/edit_vertex.gif" ), nullptr, this, ID_VERTEX_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_FACE ]      = new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/face_mode.gif" ), nullptr, this, ID_FACE_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_TRANSFORM ] = new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/transform.gif" ), nullptr, this, ID_TRANSFORM_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ this->instance.geometryMode ]->setState( true );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/group.gif" ) );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/ungroup.gif" ) );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", forge::load_fx_icon( getApp(), "resources/grid.gif" ), nullptr, &_gridHideTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/grid_up.gif" ), this, ID_GRID_UP );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/grid_down.gif" ), this, ID_GRID_DOWN );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/grid_orient.gif" ), this, ID_GRID_ALIGN );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_READONLY | TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", forge::load_fx_icon( getApp(), "resources/play.gif" ) );

	auto *hs = new FX4Splitter( middleFrame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );

	ApeCameraViewMode viewModes[ APE_EDITOR_MAX_VIEWPORTS ] = { APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_MODE_TOP, APE_CAMERA_MODE_LEFT, APE_CAMERA_MODE_FRONT };
	for ( unsigned int i = 0; i < APE_EDITOR_MAX_VIEWPORTS; ++i )
	{
		viewports[ i ] = new Viewport( hs, get_shared_gl_visual(), this, viewModes[ i ] );
	}

	owner->create();

	this->_world = world;

	if ( !worldName.empty() )
	{
		setText( "World Editor (" + worldName + ")" );
	}

	ape_editor_set_active_instance( &this->instance );
}

forge::WorldEditor::~WorldEditor() = default;

void forge::WorldEditor::create_new_object( const char *name, ApeWorldNodeType type )
{
	FXTreeItem *selectedItem = nodeTree->getCurrentItem();
	if ( selectedItem == nullptr )
	{
		return;
	}

	auto *parentNode = ( ApeWorldNode * ) nodeTree->getCurrentItem()->getData();
	if ( parentNode == nullptr )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_WARNING, "Failed to create object, no valid node selected!" );
		return;
	}

	PLVector3 pos;
	ape_grid_get_cursor_position( &instance.grid, &pos );

	static const PLColourF32 colour = ( PLColourF32 ){ 1.0f, 1.0f, 1.0f, 1.0f };

	void *data = nullptr;
	switch ( type )
	{
		default:
			break;
		case APE_WORLD_NODE_TYPE_EMPTY:
			break;
		case APE_WORLD_NODE_TYPE_BRUSH:
			data = ape_create_brush( parentNode, &pos, &pl_vecOrigin3 );
			break;
		case APE_WORLD_NODE_TYPE_LIGHT:
			data = ape_create_light( parentNode, &pos, &colour, 1.0f, APE_LIGHT_TYPE_OMNI, APE_LIGHT_FLAG_ENABLED );
			break;
		case APE_WORLD_NODE_TYPE_CAMERA:
			data = ape_create_camera( parentNode, nullptr, &pos, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
			break;
		case APE_WORLD_NODE_TYPE_ENTITY: break;
	}
	assert( data != nullptr );

	update_tree();
}

void forge::WorldEditor::update_tree()
{
	ApeWorldNode *child;
	PL_ITERATE_LINKED_LIST( child, ApeWorldNode, _world->base.children )
	{
		if ( child->type != APE_WORLD_NODE_TYPE_ROOM )
		{
			continue;
		}

		if ( roomSelectBox->findItemByData( child ) != -1 )
		{
			continue;
		}

		roomSelectBox->appendItem( child->name, child );
	}
}

long forge::WorldEditor::on_change_geometry_mode( FXObject *, FXSelector selector, void * )
{
	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_POLY_MODE:
			this->instance.geometryMode = APE_EDITOR_GEOMETRY_MODE_PLOT;
			break;
		case ID_FACE_MODE:
			this->instance.geometryMode = APE_EDITOR_GEOMETRY_MODE_FACE;
			break;
		case ID_EDGE_MODE:
			this->instance.geometryMode = APE_EDITOR_GEOMETRY_MODE_EDGE;
			break;
		case ID_VERTEX_MODE:
			this->instance.geometryMode = APE_EDITOR_GEOMETRY_MODE_VERTEX;
			break;
		case ID_TRANSFORM_MODE:
			this->instance.geometryMode = APE_EDITOR_GEOMETRY_MODE_TRANSFORM;
			break;
	}

	for ( unsigned int i = 0; i < APE_EDITOR_MAX_GEOMETRY_MODES; ++i )
	{
		geometryModeButtons[ i ]->setState( this->instance.geometryMode == i );
	}

	return TRUE;
}

long forge::WorldEditor::on_shift_grid( FXObject *, FXSelector selector, void * )
{
	PLVector3 forward;
	PlExtractMatrix4Directions( &instance.grid.transform, nullptr, &forward, nullptr );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &instance.grid.transform );

	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_GRID_UP:
		{
			PlTranslateMatrix( PlInverseVector3( forward ) );
			break;
		}
		case ID_GRID_DOWN:
		{
			PlTranslateMatrix( forward );
			break;
		}
		case ID_GRID_ALIGN:
		{
			//todo: depends on being able to select a face
			break;
		}
	}

	instance.grid.transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlPopMatrix();

	return TRUE;
}

long forge::WorldEditor::on_room_select( FXObject *, FXSelector, void * )
{
	FXint current = roomSelectBox->getCurrentItem();

	ApeRoom *room = ( ApeRoom * ) roomSelectBox->getItemData( current );
	if ( room == nullptr )
	{
		return false;
	}

	ape_world_node_set_name( ( ApeWorldNode * ) room, roomSelectBox->getItemText( current ).text() );

	for ( auto *viewport : viewports )
	{
		ape_camera_set_room( viewport->camera, room );
	}

	return true;
}

long forge::WorldEditor::on_new_room( FXObject *, FXSelector, void * )
{
	RoomCreationDialog roomCreationDialog( this );
	if ( roomCreationDialog.execute() )
	{
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////
// Room Creation Dialog

FXDEFMAP( forge::WorldEditor::RoomCreationDialog )
roomCreationMap[] = {};
FXIMPLEMENT( forge::WorldEditor::RoomCreationDialog, FXDialogBox, roomCreationMap, ARRAYNUMBER( roomCreationMap ) )

forge::WorldEditor::RoomCreationDialog::RoomCreationDialog( FXWindow *parent ) : FXDialogBox( parent, "Room Creation" )
{
}
