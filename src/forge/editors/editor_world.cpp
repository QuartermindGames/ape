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

ss::forge::editor_world::editor_world( FXTabBook *owner, const FXString &worldName, ApeWorld *world )
    : FXTabItem( owner, "World Editor" )
{
	setIcon( ss::forge::load_fx_icon( getApp(), "resources/world_editor.gif" ) );

	auto frame = new FXHorizontalFrame( owner, LAYOUT_FILL_X | LAYOUT_FILL_Y | LAYOUT_SIDE_TOP | FRAME_RAISED );
	auto leftSidebar = new FXVerticalFrame( frame, LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH | FRAME_RAISED, 0, 0, 200 );

	nodeTree = new FXTreeList( leftSidebar, nullptr, 0, LAYOUT_FILL_X | LAYOUT_FILL_Y | TREELIST_ROOT_BOXES | TREELIST_SHOWS_LINES | TREELIST_SHOWS_BOXES );

	auto *middleFrame = new FXVerticalFrame( frame, FRAME_RAISED | LAYOUT_FILL );

	auto *toolbar = new FXToolBar( middleFrame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/save.gif" ) );
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

	viewportFrame = new viewport_frame( hs, get_shared_gl_visual(), this, APE_CAMERA_MODE_PERSPECTIVE );

	//auto rightSidebar = new FXVerticalSeparator( frame, LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH, 0, 0, 200 );

	frame->create();

	this->_world = world;

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
	//TODO: in the longer term this shouldn't flush every time we update it, otherwise it's going to get expensive fast!!

	nodeTree->clearItems();
	FXTreeItem *parentItem = nodeTree->appendItem( nullptr, _world->root->name );

	FXIcon *icon = ss::forge::load_fx_icon( getApp(), "resources/world_editor.gif" );
	assert( icon != nullptr );
	//parentItem->setClosedIcon( icon );
	//parentItem->setOpenIcon( icon );

	PLLinkedListNode *node = PlGetFirstNode( _world->root->children );
	while ( node != nullptr )
	{
		auto *worldNode = ( ApeWorldNode * ) PlGetLinkedListNodeUserData( node );

		const char *iconPath;
		switch ( worldNode->type )
		{
			default:
				iconPath = "resources/node.gif";
				break;
			case APE_WORLD_NODE_TYPE_ROOM:
				iconPath = "resources/room.gif";
				break;
			case APE_WORLD_NODE_TYPE_BRUSH:
				iconPath = "resources/brush.gif";
				break;
			case APE_WORLD_NODE_TYPE_LIGHT:
				iconPath = "resources/light.gif";
				break;
			case APE_WORLD_NODE_TYPE_CAMERA:
				iconPath = "resources/camera.gif";
				break;
			case APE_WORLD_NODE_TYPE_ENTITY:
				iconPath = "resources/entity.gif";
				break;
		}

		FXTreeItem *item = nodeTree->appendItem( parentItem, worldNode->name );

		icon = ss::forge::load_fx_icon( getApp(), iconPath );
		assert( icon != nullptr );
		//item->setClosedIcon( icon );
		//item->setOpenIcon( icon );

		nodeTree->setItemData( item, worldNode );

		node = PlGetNextLinkedListNode( node );
	}
}
