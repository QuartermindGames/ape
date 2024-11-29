#include "forge.h"
#include "forge_browser_materials.h"

#include "forge_editor_world.h"
#include "forge_window_main.h"

FXDEFMAP( forge::MaterialBrowser )
materialBrowserMap[] = {
        FXMAPFUNC( SEL_COMMAND, forge::MaterialBrowser::ID_MATERIAL_LIST, forge::MaterialBrowser::on_material_select ),
};

FXIMPLEMENT( forge::MaterialBrowser, FXDialogBox, materialBrowserMap, ARRAYNUMBER( materialBrowserMap ) )

void forge::MaterialBrowser::cache_preview_callback( const char *path, void *user )
{
	MaterialBrowser *self = static_cast< MaterialBrowser * >( user );

	MaterialPreview *preview = new MaterialPreview;

	preview->path = path;
	preview->icon = ape_material_load_preview( path );
	if ( preview->icon == nullptr )
	{
		printf( "Failed to load icon preview (%s): %s\n", path, PlGetError() );
		return;
	}

	self->materialPreviews.push_back( preview );
}

forge::MaterialBrowser::MaterialBrowser( FX::FXWindow *parent )
    : FXDialogBox( parent, "Material Browser", DECOR_TITLE | DECOR_CLOSE | DECOR_BORDER | DECOR_RESIZE | DECOR_MENU )
{
	setWidth( 640 );
	setHeight( 480 );

	setPadLeft( 0 );
	setPadRight( 0 );
	setPadBottom( 0 );
	setPadTop( 0 );

	// do a scan for all the available materials
	AcmBranch *root  = ape_editor_get_config();
	AcmBranch *child = acm_get_child_by_name( root, "materialPaths" );
	if ( child != nullptr )
	{
		child = acm_get_first_child( child );
		while ( child != nullptr )
		{
			PLPath buf;
			acm_branch_get_string( child, buf, sizeof( buf ) );
			PlScanDirectory( buf, "n", cache_preview_callback, true, this );
			child = acm_get_next_child( child );
		}
	}

	FXVerticalFrame *frame = new FXVerticalFrame( this, LAYOUT_FILL | FRAME_NORMAL );

	materialList = new FXIconList( frame, this, ID_MATERIAL_LIST,
	                               ICONLIST_BIG_ICONS | ICONLIST_ROWS | ICONLIST_COLUMNS | ICONLIST_AUTOSIZE | ICONLIST_BROWSESELECT | LAYOUT_FILL );
	materialList->appendHeader( "Name", nullptr, 200 );
	materialList->appendHeader( "Size", nullptr, 60 );
	materialList->appendHeader( "Group", nullptr, 50 );

	for ( auto preview : materialPreviews )
	{
		FXIcon  *icon = new FXIcon( getApp() );
		FXColor *data = ( FXColor * ) PlGetImageData( preview->icon, 0, 0 );
		icon->setData( data, IMAGE_KEEP | IMAGE_ALPHACOLOR, ( int ) preview->icon->width, ( int ) preview->icon->height );
		icon->create();

		materialList->appendItem( preview->path.c_str(), icon, nullptr, preview );
	}

	materialList->setSortFunc( FX::FXIconList::ascendingCase );
	materialList->sortItems();

	ApeMaterial *defaultMaterial = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR );
	FXint        itemIndex       = materialList->findItemByData( defaultMaterial );
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

	ApeEditorInstance *instance = ape_editor_get_active_instance();
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
