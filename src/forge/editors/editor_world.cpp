// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: World editor tab
// Author:  Mark E. Sowden

#include "editor_world.h"

#include "../viewport_frame.h"

#include "yin/core_entity.h"

FXDEFMAP( ss::forge::editor_world )
worldEditorMap[] = {
        FXMAPFUNC( SEL_COMMAND, ss::forge::editor_world::ID_BRUSH_MODE, ss::forge::editor_world::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::editor_world::ID_FACE_MODE, ss::forge::editor_world::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::editor_world::ID_EDGE_MODE, ss::forge::editor_world::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::editor_world::ID_VERTEX_MODE, ss::forge::editor_world::on_change_geometry_mode ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::editor_world::ID_TRANSFORM_MODE, ss::forge::editor_world::on_change_geometry_mode ),
};
FXIMPLEMENT( ss::forge::editor_world, EditorTab, worldEditorMap, ARRAYNUMBER( worldEditorMap ) )

ss::forge::editor_world::editor_world( FXTabBook *owner, const FXString &worldName, ApeWorld *world )
    : EditorTab( owner, "World Editor", forge_cachedIcons[ FORGE_ICON_TYPE_WORLD ] ),
      _gridSizeTarget( this->instance.gridScale ),
      _gridHideTarget( this->instance.gridVisible )
{
	auto frame = new FXHorizontalFrame( owner, LAYOUT_FILL_X | LAYOUT_FILL_Y | LAYOUT_SIDE_TOP | FRAME_RAISED );
	auto leftSidebar = new FXVerticalFrame( frame, LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH | FRAME_RAISED, 0, 0, 200 );

	nodeTree = new FXTreeList( leftSidebar, nullptr, 0, LAYOUT_FILL_X | LAYOUT_FILL_Y | TREELIST_ROOT_BOXES | TREELIST_SHOWS_LINES | TREELIST_SHOWS_BOXES );

	auto *middleFrame = new FXVerticalFrame( frame, FRAME_RAISED | LAYOUT_FILL );

	auto *toolbar = new FXToolBar( middleFrame, FRAME_RAISED | FRAME_THICK );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/save.gif" ) );
	new FXVerticalSeparator( toolbar );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_BRUSH ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/brush_mode.gif" ), nullptr, this, ID_BRUSH_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_FACE ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/face_mode.gif" ), nullptr, this, ID_FACE_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_EDGE ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/edge_mode.gif" ), nullptr, this, ID_EDGE_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_VERTEX ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/vertex_mode.gif" ), nullptr, this, ID_VERTEX_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ APE_EDITOR_GEOMETRY_MODE_TRANSFORM ] = new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/transform.gif" ), nullptr, this, ID_TRANSFORM_MODE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_TOOLBAR | TOGGLEBUTTON_NORMAL );
	geometryModeButtons[ this->instance.geometryMode ]->setState( true );

	new FXVerticalSeparator( toolbar );
	new FXToggleButton( toolbar, "", "", ss::forge::load_fx_icon( getApp(), "resources/grid.gif" ), 0, &_gridHideTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/grid_up.gif" ) );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/grid_down.gif" ) );
	new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/grid_orient.gif" ) );
	new FXTextField( toolbar, 4, &_gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_READONLY | TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );

	//new FXVerticalSeparator( toolbar );
	//new FXButton( toolbar, "", ss::forge::load_fx_icon( getApp(), "resources/play.gif" ) );

#if 0

    auto *hs = new FXVerticalFrame( middleFrame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	hs->setPadBottom( 0 );
	hs->setPadLeft( 0 );
	hs->setPadRight( 0 );
	hs->setPadTop( 0 );
	new viewport_frame( hs, get_shared_gl_visual(), this, APE_CAMERA_MODE_PERSPECTIVE );

#else

	auto *hs = new FX4Splitter( middleFrame, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );

	new viewport_frame( hs, get_shared_gl_visual(), this, APE_CAMERA_MODE_PERSPECTIVE );
	new viewport_frame( hs, get_shared_gl_visual(), this, APE_CAMERA_MODE_TOP );
	new viewport_frame( hs, get_shared_gl_visual(), this, APE_CAMERA_MODE_LEFT );
	new viewport_frame( hs, get_shared_gl_visual(), this, APE_CAMERA_MODE_FRONT );

#endif

	//auto rightSidebar = new FXVerticalSeparator( frame, LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH, 0, 0, 200 );

	frame->create();

	this->_world = world;

	if ( !worldName.empty() )
	{
		setText( "World Editor (" + worldName + ")" );
	}

	ape_editor_set_active_instance( &this->instance );
}

ss::forge::editor_world::~editor_world() = default;

void ss::forge::editor_world::create_new_entity( ApeWorldNode *parent )
{
	ape_entity_create( "test", nullptr );

	update_tree();
}

void ss::forge::editor_world::update_tree()
{
	FXTreeItem *parentItem = nodeTree->findItem( _world->root->name );
	if ( parentItem == nullptr )
	{
		parentItem = nodeTree->appendItem( nullptr, _world->root->name );
		parentItem->setClosedIcon( forge_cachedIcons[ FORGE_ICON_TYPE_WORLD ] );
		parentItem->setOpenIcon( forge_cachedIcons[ FORGE_ICON_TYPE_WORLD ] );
		parentItem->setExpanded( true );
	}

	PLLinkedListNode *node = PlGetFirstNode( _world->root->children );
	while ( node != nullptr )
	{
		auto *worldNode = ( ApeWorldNode * ) PlGetLinkedListNodeUserData( node );
		FXTreeItem *item = nodeTree->findItemByData( worldNode );
		if ( item == nullptr )
		{
			ForgeIconType iconType;
			switch ( worldNode->type )
			{
				default:
					iconType = FORGE_ICON_TYPE_NODE;
					break;
				case APE_WORLD_NODE_TYPE_ROOM:
					iconType = FORGE_ICON_TYPE_ROOM;
					break;
				case APE_WORLD_NODE_TYPE_BRUSH:
					iconType = FORGE_ICON_TYPE_BRUSH;
					break;
				case APE_WORLD_NODE_TYPE_LIGHT:
					iconType = FORGE_ICON_TYPE_LIGHT;
					break;
				case APE_WORLD_NODE_TYPE_CAMERA:
					iconType = FORGE_ICON_TYPE_CAMERA;
					break;
				case APE_WORLD_NODE_TYPE_ENTITY:
					iconType = FORGE_ICON_TYPE_ENTITY;
					break;
			}

			item = nodeTree->appendItem( parentItem, worldNode->name );
			nodeTree->setItemData( item, worldNode );

			item->setClosedIcon( forge_cachedIcons[ iconType ] );
			item->setOpenIcon( forge_cachedIcons[ iconType ] );
			item->setExpanded( true );
		}

		node = PlGetNextLinkedListNode( node );
	}
}

long ss::forge::editor_world::on_change_geometry_mode( FXObject *, FXSelector selector, void * )
{
	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_BRUSH_MODE:
			this->instance.geometryMode = APE_EDITOR_GEOMETRY_MODE_BRUSH;
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

long ss::forge::editor_world::on_shift_grid( FXObject *, FXSelector selector, void * )
{
	switch ( FXSELID( selector ) )
	{
		default:
			break;
		case ID_GRID_UP:
			break;
		case ID_GRID_DOWN:
			break;
		case ID_GRID_ROTATE:
			break;
	}

	return TRUE;
}
