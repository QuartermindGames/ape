// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: World editor tab
// Author:  Mark E. Sowden

#include "editor_world.h"

#include "../forge_viewport_frame.h"

#include "yin/core_entity.h"

FXDEFMAP( ss::forge::editor_world )
worldEditorMap[] = {

};
FXIMPLEMENT( ss::forge::editor_world, FXTabItem, worldEditorMap, ARRAYNUMBER( worldEditorMap ) )

ss::forge::editor_world::editor_world( FXTabBook *owner, const FXString &worldName, ApeWorld *world ) : FXTabItem( owner, "World Editor" )
{
	setIcon( ss::forge::load_fx_icon( getApp(), "resources/world_editor.gif" ) );

	auto frame = new FXHorizontalFrame( owner, LAYOUT_FILL_X | LAYOUT_FILL_Y | LAYOUT_SIDE_TOP );
	auto leftSidebar = new FXVerticalFrame( frame, LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH | FRAME_RAISED, 0, 0, 200 );

	nodeTree = new FXTreeList( leftSidebar, nullptr, 0, LAYOUT_FILL_X | LAYOUT_FILL_Y | TREELIST_ROOT_BOXES | TREELIST_SHOWS_LINES | TREELIST_SHOWS_BOXES );

	auto *middleFrame = new FXVerticalFrame( frame, LAYOUT_FILL );

	auto *toolbar = new FXToolBar( middleFrame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/save.gif" ) );
#if 1
	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/new_room.gif" ) );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/new_brush.gif" ) );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/new_light.gif" ) );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/new_camera.gif" ) );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/new_entity.gif" ) );
#endif
	new FXVerticalSeparator( toolbar );
	_editModeButtons[ APE_EDITOR_GEOMETRY_MODE_TRANSFORM ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/transform.gif" ), nullptr, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	_editModeButtons[ APE_EDITOR_GEOMETRY_MODE_FACE ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/face_mode.gif" ), nullptr, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	_editModeButtons[ APE_EDITOR_GEOMETRY_MODE_EDGE ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/edge_mode.gif" ), nullptr, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	_editModeButtons[ APE_EDITOR_GEOMETRY_MODE_VERTEX ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/vertex_mode.gif" ), nullptr, this, 0, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );

	ApeEditorState *editorState = ape_editor_get_state();
	_editModeButtons[ editorState->geometryMode ]->setState( true );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/grid.gif" ), 0, &_gridSizeTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	new FXVerticalSeparator( toolbar );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/play.gif" ) );

	auto *hs = new FXVerticalFrame( middleFrame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	hs->setPadBottom( 0 );
	hs->setPadLeft( 0 );
	hs->setPadRight( 0 );
	hs->setPadTop( 0 );

	viewportFrame = new viewport_frame( hs, get_shared_gl_visual(), APE_CAMERA_MODE_PERSPECTIVE );

	auto rightSidebar = new FXVerticalSeparator( frame, LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH, 0, 0, 200 );

	frame->create();

	if ( !worldName.empty() )
	{
		setText( "World Editor (" + worldName + ")" );
	}
}

ss::forge::editor_world::~editor_world() = default;

void ss::forge::editor_world::create_new_entity( ApeWorldNode *parent )
{
	ape_entity_create( "test", nullptr );

	update_tree();
}

void ss::forge::editor_world::update_tree()
{
	nodeTree->clearItems();

	PLLinkedListNode *node = PlGetFirstNode( _world->root->children );
	while ( node != nullptr )
	{
		auto *worldNode = ( ApeWorldNode * ) PlGetLinkedListNodeUserData( node );

		FXTreeItem *parentNode = nullptr;
		switch ( worldNode->type )
		{
			default:
				break;
			case APE_WORLD_NODE_TYPE_ROOM:
				parentNode = nodeTree->appendItem( nullptr, worldNode->name, ss::forge::load_fx_icon( getApp(), "resources/room.gif" ) );
				break;
			case APE_WORLD_NODE_TYPE_BRUSH:
				parentNode = nodeTree->appendItem( nullptr, worldNode->name, ss::forge::load_fx_icon( getApp(), "resources/brush.gif" ) );
				break;
			case APE_WORLD_NODE_TYPE_LIGHT:
				parentNode = nodeTree->appendItem( nullptr, worldNode->name, ss::forge::load_fx_icon( getApp(), "resources/light.gif" ) );
				break;
			case APE_WORLD_NODE_TYPE_CAMERA:
				parentNode = nodeTree->appendItem( nullptr, worldNode->name, ss::forge::load_fx_icon( getApp(), "resources/camera.gif" ) );
				break;
			case APE_WORLD_NODE_TYPE_ENTITY:
				parentNode = nodeTree->appendItem( nullptr, worldNode->name, ss::forge::load_fx_icon( getApp(), "resources/entity.gif" ) );
				break;
		}

		if ( parentNode != nullptr )
		{
			nodeTree->setItemData( parentNode, worldNode );
		}

		node = PlGetNextLinkedListNode( node );
	}
}