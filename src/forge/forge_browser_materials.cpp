#include "forge.h"
#include "forge_browser_materials.h"

FXDEFMAP( forge::MaterialBrowser )
materialBrowserMap[] = {};

FXIMPLEMENT( forge::MaterialBrowser, FXDialogBox, materialBrowserMap, ARRAYNUMBER( materialBrowserMap ) )

forge::MaterialBrowser::MaterialBrowser( FX::FXWindow *parent )
    : FXDialogBox( parent, "Material Browser", DECOR_ALL )
{
	setWidth( 640 );
	setHeight( 480 );

	setPadLeft( 0 );
	setPadRight( 0 );
	setPadBottom( 0 );
	setPadTop( 0 );

	unsigned int  numMaterials;
	ApeMaterial **materials = ape_editor_get_available_materials( &numMaterials );

	FXVerticalFrame *frame = new FXVerticalFrame( this, LAYOUT_FILL | FRAME_NORMAL );

	materialList = new FXIconList( frame, nullptr, 0,
	                               ICONLIST_BIG_ICONS | ICONLIST_ROWS | ICONLIST_COLUMNS | ICONLIST_AUTOSIZE | ICONLIST_BROWSESELECT | LAYOUT_FILL );
	materialList->appendHeader( "Name", nullptr, 200 );
	materialList->appendHeader( "Size", nullptr, 60 );
	materialList->appendHeader( "Group", nullptr, 50 );
	for ( unsigned int i = 0; i < numMaterials; ++i )
	{
		PLImage *preview = ape_material_get_preview( materials[ i ] );
		if ( preview == nullptr )
		{
			continue;
		}

		FXIcon *icon = new FXIcon( getApp() );
		icon->setData( ( FXColor * ) PlGetImageData( preview, 0, 0 ), IMAGE_KEEP | IMAGE_ALPHACOLOR, ( int ) preview->width, ( int ) preview->height );
		icon->create();

		const char *path = ape_material_get_path( materials[ i ] );
		materialList->appendItem( path, icon, nullptr, &materials[ i ] );
	}

	materialList->setSortFunc( FX::FXIconList::ascendingCase );
	materialList->sortItems();
}

ApeMaterial *forge::MaterialBrowser::get_current()
{
	return ( ApeMaterial * ) ( materialList->getItemData( materialList->getCurrentItem() ) );
}

void forge::MaterialBrowser::create()
{
	FXTopWindow::create();

	show();
}
