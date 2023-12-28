// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "ProjectDialog.h"
#include "common_project.h"

#include "3rdparty/fox/src/icons.h"

std::map< std::string, ss::forge::Project * > ss::forge::ProjectDialog::projects;

FXDEFMAP( ss::forge::ProjectDialog )
projectDialogMap[] = {
        FXMAPFUNC( SEL_COMMAND, ss::forge::ProjectDialog::ID_SELECT_PROJECT, ss::forge::ProjectDialog::on_select_project ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::ProjectDialog::ID_ACCEPT, ss::forge::ProjectDialog::on_accept ),
};

FXIMPLEMENT( ss::forge::ProjectDialog, FXDialogBox, projectDialogMap, ARRAYNUMBER( projectDialogMap ) )

ss::forge::ProjectDialog::ProjectDialog( FX::FXWindow *parent )
    : FXDialogBox( parent, "Project Setup" )
{
	setWidth( baseWidth );

	if ( projects.empty() )
	{
		// do a quick scan to see what projects are available
		std::string localPath = "local://" + std::string( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ] );
		PlScanDirectory( localPath.c_str(), "prj.n", register_project_callback, true, this );
	}

	auto *vf = new FXVerticalFrame( this, LAYOUT_FILL );

	static FXGIFIcon folderIcon( getApp(), FX::minifolder );
	listBox = new FXListBox( vf, this, ID_SELECT_PROJECT, FRAME_SUNKEN | FRAME_THICK | LISTBOX_NORMAL | LAYOUT_FILL_X );
	for ( const auto &i : projects )
		listBox->appendItem( FXString( i.second->name.c_str() ) + " (" + i.second->internalName.c_str() + ")", i.second->icon, i.second );

	static FXGIFIcon newFolderIcon( getApp(), FX::foldernew );
	listBox->appendItem( "Create new project", &newFolderIcon, nullptr );
	listBox->setNumVisible( PlClamp( 4, listBox->getNumItems(), 8 ) );

	projectNameField = new FXTextField( vf, 1, nullptr, 0, TEXTFIELD_NORMAL | LAYOUT_FILL_X );
	projectNameField->setText( defaultName );
	if ( !projects.empty() )
		projectNameField->hide();

	new FXHorizontalSeparator( vf );

	auto *hf = new FXHorizontalFrame( vf, LAYOUT_FILL | LAYOUT_RIGHT );
	new FXButton( hf, "Accept", nullptr, this, ID_ACCEPT, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
	new FXButton( hf, "Cancel", nullptr, this, ID_CANCEL, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
}

void ss::forge::ProjectDialog::register_project_callback( const char *path, void *data )
{
	const char *filename = PlGetFileName( path );
	if ( filename == nullptr )
	{
		EDITOR_PRINT( "Failed to get filename: %s\n", PlGetError() );
		return;
	}

	const char *c = strchr( filename, '.' );
	if ( c == nullptr )
	{
		EDITOR_PRINT( "Failed to get filename terminator (%s)!\n", path );
		return;
	}

	NdBranch *root = ndLoadFile( path, "project" );
	if ( root == nullptr )
		return;

	if ( ndGetBoolByName( root, "visibleInEditor", true ) )
	{
		const char *name = ndGetStringByName( root, "name", nullptr );
		if ( name == nullptr )
		{
			FXMessageBox::warning( FXApp::instance(), MBOX_OK,
			                       "Warning",
			                       "Encountered a project without a name!\n"
			                       "%s",
			                       path );
		}
		else
		{
			auto *project = PL_NEW( Project );
			project->name = name;
			project->internalName.assign( filename, c - filename );

			static FXGIFIcon folderIcon( FXApp::instance(), FX::minifolder );
			if ( project->icon == nullptr )
				project->icon = &folderIcon;

			projects.emplace( project->internalName, project );
		}
	}

	ndDestroyBranch( root );
}

long ss::forge::ProjectDialog::on_select_project( FXObject *, FXSelector, void * )
{
	// show or hide the name field, depending on if it's a valid item or not
	if ( listBox->getItemData( listBox->getCurrentItem() ) == nullptr )
	{
		projectNameField->show();
		projectNameField->setText( defaultName );
	}
	else
	{
		projectNameField->hide();
		projectNameField->setText( "" );
	}

	// and then resize, otherwise the buttons aren't visible -_-;
	resize( baseWidth, getDefaultHeight() );

	return true;
}

long ss::forge::ProjectDialog::on_accept( FXObject *obj, FXSelector sel, void *ptr )
{
	// urgh, check if we have a valid project selected and if not,
	// that the user has entered *something*
	forge::editorProject = ( Project * ) listBox->getItemData( listBox->getCurrentItem() );
	if ( ( forge::editorProject == nullptr ) && ( projectNameField->getText() != defaultName ) )
	{
		PLPath folderName;
		forge::editorProject = forge::create_project(
		        std::string( projectNameField->getText().text() ),
		        PlSetupPath( folderName, true, "%s", projectNameField->getText().text() ) );
	}

	forge::open_project( forge::editorProject->internalName.c_str() );

	return FXDialogBox::onCmdAccept( obj, sel, ptr );
}
