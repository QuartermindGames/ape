// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "forge.h"
#include "forge_browser_materials.h"

#include "forge_editor_world.h"
#include "forge_window_main.h"

#include <sys/wait.h>

FXDEFMAP( forge::MaterialBrowser )
materialBrowserMap[] = {
        FXMAPFUNC( SEL_COMMAND, forge::MaterialBrowser::ID_MATERIAL_LIST, forge::MaterialBrowser::on_material_select ),
        FXMAPFUNC( SEL_COMMAND, forge::MaterialBrowser::ID_MATERIAL_EDIT, forge::MaterialBrowser::on_material_edit ),
        FXMAPFUNC( SEL_COMMAND, forge::MaterialBrowser::ID_MATERIAL_GROUP_LIST, forge::MaterialBrowser::on_material_group_select ),
        FXMAPFUNC( SEL_COMMAND, forge::MaterialBrowser::ID_MATERIAL_ICON_SIZE, forge::MaterialBrowser::on_material_icon_size ),
        FXMAPFUNC( SEL_CHANGED, forge::MaterialBrowser::ID_MATERIAL_FILTER, forge::MaterialBrowser::on_material_filter ),
        FXMAPFUNC( SEL_COMMAND, forge::MaterialBrowser::ID_MATERIAL_APPLY, forge::MaterialBrowser::on_material_apply ),
};

FXIMPLEMENT( forge::MaterialBrowser, FXDialogBox, materialBrowserMap, ARRAYNUMBER( materialBrowserMap ) )

void forge::MaterialBrowser::cache_preview_callback( const char *path, void *user )
{
	MaterialBrowser *self = static_cast< MaterialBrowser * >( user );

	std::string folder = path;
	folder.erase( folder.find_last_of( '/' ) );
	self->materialPreviewGroups.emplace( folder, std::vector< MaterialPreview * >() );
	auto &group = self->materialPreviewGroups.at( folder );

	MaterialPreview *preview = new MaterialPreview;

	preview->path = path;
	preview->icon = ape_material_load_preview( path );
	if ( preview->icon == nullptr )
	{
		printf( "Failed to load icon preview (%s): %s\n", path, PlGetError() );
		return;
	}

	preview->smallIcon = PlResizeImage( preview->icon, 16, 16 );
	if ( preview->smallIcon == nullptr )
	{
		printf( "Failed to resize icon preview (%s): %s\n", path, PlGetError() );
		preview->smallIcon = preview->icon;
	}

	preview->name = std::string( preview->path ).substr( preview->path.find_last_of( '/' ) + 1 );
	preview->name.erase( preview->name.find_last_of( '.' ) );
	if ( preview->name.find( ".mat" ) != std::string::npos )
	{
		preview->name.erase( preview->name.find_last_of( '.' ) );
	}

	group.push_back( preview );
}

forge::MaterialBrowser::MaterialBrowser( FXWindow *parent )
    : FXDialogBox( parent, "Material Browser", DECOR_TITLE | DECOR_CLOSE | DECOR_BORDER | DECOR_RESIZE | DECOR_MENU )
{
	setWidth( 316 );
	setHeight( 500 );

	setPadLeft( 0 );
	setPadRight( 0 );
	setPadBottom( 0 );
	setPadTop( 0 );

	// do a scan for all the available materials
	AcmBranch *root = ape_editor_get_config();
	if ( AcmBranch *child = acm_get_child_by_name( root, "materialPaths" ); child != nullptr )
	{
		child = acm_get_first_child( child );
		while ( child != nullptr )
		{
			PLPath buf;
			acm_branch_get_string( child, buf, sizeof( buf ) );
			PlScanDirectory( buf, "n", cache_preview_callback, false, this );
			child = acm_get_next_child( child );
		}
	}

	FXVerticalFrame *frame = new FXVerticalFrame( this, LAYOUT_FILL | FRAME_NONE );
	frame->setPadLeft( 0 );
	frame->setPadRight( 0 );
	frame->setPadBottom( 0 );
	frame->setPadTop( 0 );

	FXToolBar *toolBar               = new FXToolBar( frame, LAYOUT_FILL_X | FRAME_RAISED | FRAME_THICK );
	viewModes[ VIEW_MODE_LIST ]      = new FXToggleButton( toolBar, "", "", load_fx_icon( getApp(), "resources/list_details.png" ), nullptr, this, ID_MATERIAL_ICON_SIZE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	viewModes[ VIEW_MODE_ICONS ]     = new FXToggleButton( toolBar, "", "", load_fx_icon( getApp(), "resources/list_previews.png" ), nullptr, this, ID_MATERIAL_ICON_SIZE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	viewModes[ VIEW_MODE_BIG_ICONS ] = new FXToggleButton( toolBar, "", "", load_fx_icon( getApp(), "resources/list_previews_large.png" ), nullptr, this, ID_MATERIAL_ICON_SIZE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	viewModes[ VIEW_MODE_BIG_ICONS ]->setState( true );
	new FXVerticalSeparator( toolBar );
	materialGroupList = new FXListBox( toolBar, this, ID_MATERIAL_GROUP_LIST );
	materialGroupList->setNumVisible( 8 );
	for ( auto &group : materialPreviewGroups )
	{
		materialGroupList->appendItem( group.first.c_str() );
	}
	materialGroupList->setCurrentItem( 0 );

	materialList = new FXIconList( frame, this, ID_MATERIAL_LIST,
	                               ICONLIST_BIG_ICONS | ICONLIST_ROWS | ICONLIST_COLUMNS | ICONLIST_AUTOSIZE | ICONLIST_BROWSESELECT | LAYOUT_FILL );
	materialList->appendHeader( "Name", nullptr, 200 );
	materialList->appendHeader( "Width", nullptr, 60 );
	materialList->appendHeader( "Height", nullptr, 60 );

	materialList->setSortFunc( FXIconList::ascendingCase );
	materialList->sortItems();

	new FXTextField( frame, 20, this, ID_MATERIAL_FILTER, TEXTFIELD_NORMAL | LAYOUT_FILL_X );

	FXHorizontalFrame *hframe = new FXHorizontalFrame( frame, LAYOUT_FILL_X | FRAME_NONE );
	hframe->setPadLeft( 0 );
	hframe->setPadRight( 0 );
	hframe->setPadBottom( 0 );
	hframe->setPadTop( 0 );

	new FXButton( hframe, "Edit", nullptr, this, ID_MATERIAL_EDIT, LAYOUT_FILL_X | FRAME_RAISED | FRAME_THICK );
	new FXButton( hframe, "Apply", nullptr, this, ID_MATERIAL_APPLY, LAYOUT_FILL_X | FRAME_RAISED | FRAME_THICK );

	update_material_list();
}

void forge::MaterialBrowser::update_material_list( const std::string &filter )
{
	const FXint item      = materialGroupList->getCurrentItem();
	FXString    groupName = materialGroupList->getItemText( item );
	if ( groupName.empty() )
	{
		return;
	}

	materialList->clearItems();

	auto &group = materialPreviewGroups[ groupName.text() ];
	for ( auto preview : group )
	{
		if ( !filter.empty() && preview->name.find( filter ) == std::string::npos )
		{
			continue;
		}

		FXIcon *bigIcon = new FXIcon( getApp() );
		bigIcon->setData( reinterpret_cast< FXColor * >( preview->icon->data[ 0 ] ), IMAGE_KEEP | IMAGE_ALPHACOLOR,
		                  static_cast< int >( preview->icon->width ),
		                  static_cast< int >( preview->icon->height ) );
		bigIcon->create();

		FXIcon *smallIcon = new FXIcon( getApp() );
		smallIcon->setData( reinterpret_cast< FXColor * >( preview->smallIcon->data[ 0 ] ), IMAGE_KEEP | IMAGE_ALPHACOLOR, 16, 16 );
		smallIcon->create();

		materialList->appendItem( preview->name.c_str(), bigIcon, smallIcon, preview );
	}

	const ApeMaterial *defaultMaterial = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR );
	const FXint        itemIndex       = materialList->findItemByData( defaultMaterial );
	materialList->setCurrentItem( itemIndex > -1 ? itemIndex : 0 );
}

const char *forge::MaterialBrowser::get_current() const
{
	FXint item = materialList->getCurrentItem();
	if ( item == -1 )
	{
		return nullptr;
	}

	const MaterialPreview *preview = static_cast< MaterialPreview * >( materialList->getItemData( item ) );
	if ( preview == nullptr )
	{
		return nullptr;
	}

	return preview->path.c_str();
}

void forge::MaterialBrowser::create()
{
	FXDialogBox::create();

	show();
}

long forge::MaterialBrowser::on_material_select( FXObject *, FXSelector, void * )
{
	const char *currentPath = get_current();
	if ( currentPath == nullptr )
	{
		return 0;
	}

	const ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr || instance->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return 0;
	}

	ApeBrushFace *face;
	COM_ITERATE_LINKED_LIST( face, instance->selectedObjects, i )
	{
		// check if it's set already
		const char *oldPath = ape_material_get_path( face->material );
		if ( oldPath != nullptr && strcmp( oldPath, currentPath ) == 0 )
		{
			continue;
		}

		ApeMaterial *material = ape_material_cache( currentPath, APE_CACHE_GROUP_WORLD, false );
		if ( material == nullptr )
		{
			continue;
		}

		ape_brush_face_apply_material( face, material );
	}

	return 0;
}

long forge::MaterialBrowser::on_material_edit( FXObject *, FXSelector, void * )
{
	const char *path = get_current();
	if ( path == nullptr )
	{
		return 0;
	}

	PLPath resolvedPath;
	PlResolveVirtualPath( path, resolvedPath, sizeof( resolvedPath ) );

#if !defined( _WIN32 )
	pid_t pid = fork();
	if ( pid == 0 )
	{
		execl( "/usr/bin/xdg-open", "xdg-open", resolvedPath, nullptr );
		printf( "Failed to open material for edit: %s\n", strerror( errno ) );
		exit( EXIT_FAILURE );
	}

	int status;
	if ( waitpid( pid, &status, 0 ) == -1 )
	{
		printf( "Failed to open material for edit: %s\n", strerror( errno ) );
		return -1;
	}

	if ( WIFEXITED( status ) && WEXITSTATUS( status ) != 0 )
	{
		printf( "Child process returned a non-zero exit code (%d)\n", WEXITSTATUS( status ) );
		return -1;
	}
#endif

	return 0;
}

long forge::MaterialBrowser::on_material_group_select( FXObject *, FXSelector, void * )
{
	update_material_list();
	return TRUE;
}

long forge::MaterialBrowser::on_material_icon_size( FXObject *obj, FXSelector, void * )
{
	FXuint          style  = 0;
	FXToggleButton *button = static_cast< FXToggleButton * >( obj );
	for ( uint i = 0; i < VIEW_MODE_COUNT; ++i )
	{
		viewModes[ i ]->setState( viewModes[ i ] == button );
		if ( viewModes[ i ] == button )
		{
			switch ( i )
			{
				case VIEW_MODE_LIST:
					style = ICONLIST_ROWS | ICONLIST_COLUMNS | ICONLIST_AUTOSIZE | ICONLIST_BROWSESELECT;
					break;
				case VIEW_MODE_ICONS:
					style = ICONLIST_MINI_ICONS | ICONLIST_ROWS | ICONLIST_COLUMNS | ICONLIST_AUTOSIZE | ICONLIST_BROWSESELECT;
					break;
				case VIEW_MODE_BIG_ICONS:
					style = ICONLIST_BIG_ICONS | ICONLIST_ROWS | ICONLIST_COLUMNS | ICONLIST_AUTOSIZE | ICONLIST_BROWSESELECT;
					break;
				default:
					assert( 0 );
			}
		}
	}

	materialList->setListStyle( style );

	return TRUE;
}

long forge::MaterialBrowser::on_material_filter( FXObject *obj, FXSelector, void * )
{
	const FXTextField *field = static_cast< FXTextField * >( obj );
	const FXString     text  = field->getText();

	update_material_list( text.text() );

	return TRUE;
}

long forge::MaterialBrowser::on_material_apply( FXObject *, FXSelector, void * )
{
	const char *currentPath = get_current();
	if ( currentPath == nullptr )
	{
		return 0;
	}

	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		return FALSE;
	}

	if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
	{
		ApeWorldNode *node;
		COM_ITERATE_LINKED_LIST( node, instance->selectedObjects, i )
		{
			if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
			{
				continue;
			}

			ApeBrush *brush = reinterpret_cast< ApeBrush * >( node );
			for ( uint j = 0; j < brush->numFaces; ++j )
			{
				ApeBrushFace *face = &brush->faces[ j ];

				// check if it's set already
				const char *oldPath = ape_material_get_path( face->material );
				if ( oldPath != nullptr && strcmp( oldPath, currentPath ) == 0 )
				{
					continue;
				}

				ApeMaterial *material = ape_material_cache( currentPath, APE_CACHE_GROUP_WORLD, false );
				if ( material == nullptr )
				{
					continue;
				}

				ape_brush_face_apply_material( face, material );
			}
		}
	}
	else if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		ApeBrushFace *face;
		COM_ITERATE_LINKED_LIST( face, instance->selectedObjects, i )
		{
			// check if it's set already
			const char *oldPath = ape_material_get_path( face->material );
			if ( oldPath != nullptr && strcmp( oldPath, currentPath ) == 0 )
			{
				continue;
			}

			ApeMaterial *material = ape_material_cache( currentPath, APE_CACHE_GROUP_WORLD, false );
			if ( material == nullptr )
			{
				continue;
			}

			ape_brush_face_apply_material( face, material );
		}
	}

	return TRUE;
}
