// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#include "forge_project_dialog.h"
#include "common_project.h"

#include "3rdparty/fox/src/icons.h"

std::map< std::string, forge::Project * > forge::ProjectDialog::projects;

FXDEFMAP( forge::ProjectDialog )
projectDialogMap[] = {
        FXMAPFUNC( SEL_COMMAND, forge::ProjectDialog::ID_SELECT_PROJECT, forge::ProjectDialog::on_select_project ),
        FXMAPFUNC( SEL_COMMAND, forge::ProjectDialog::ID_ACCEPT, forge::ProjectDialog::on_accept ),
};

FXIMPLEMENT( forge::ProjectDialog, FXDialogBox, projectDialogMap, ARRAYNUMBER( projectDialogMap ) )

forge::ProjectDialog::ProjectDialog( FX::FXWindow *parent )
    : FXDialogBox( parent, "Project Setup" )
{
	setWidth( baseWidth );

	if ( projects.empty() )
	{
		// do a quick scan to see what projects are available
		std::string localPath = "local://" + std::string( forge::cachedPaths[ forge::PATH_PROJECTS ] );
		PlScanDirectory( localPath.c_str(), "prj.n", register_project_callback, true, this );
	}

	auto *vf = new FXVerticalFrame( this, LAYOUT_FILL );

	static FXGIFIcon folderIcon( getApp(), FX::minifolder );
	listBox = new FXListBox( vf, this, ID_SELECT_PROJECT, FRAME_SUNKEN | FRAME_THICK | LISTBOX_NORMAL | LAYOUT_FILL_X );
	for ( const auto &i : projects )
	{
		listBox->appendItem( FXString( i.second->name.c_str() ) + " (" + i.second->internalName.c_str() + ")", i.second->icon, i.second );
	}

	static FXGIFIcon newFolderIcon( getApp(), FX::foldernew );
	listBox->appendItem( "Create new project", &newFolderIcon, nullptr );
	listBox->setNumVisible( PlClamp( 4, listBox->getNumItems(), 8 ) );

	projectNameField = new FXTextField( vf, 1, nullptr, 0, TEXTFIELD_NORMAL | LAYOUT_FILL_X );
	projectNameField->setText( defaultName );
	if ( !projects.empty() )
	{
		projectNameField->hide();
	}

	new FXHorizontalSeparator( vf );

	auto *hf = new FXHorizontalFrame( vf, LAYOUT_FILL | LAYOUT_RIGHT );
	new FXButton( hf, "Accept", nullptr, this, ID_ACCEPT, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
	new FXButton( hf, "Cancel", nullptr, this, ID_CANCEL, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
}

void forge::ProjectDialog::register_project_callback( const char *path, void *data )
{
	const char *filename = PlGetFileName( path );
	if ( filename == nullptr )
	{
		forge_warning_( "Failed to get filename: %s\n", PlGetError() );
		return;
	}

	const char *c = strchr( filename, '.' );
	if ( c == nullptr )
	{
		forge_warning_( "Failed to get filename terminator (%s)!\n", path );
		return;
	}

	AcmBranch *root = acm_load_file( path, "project" );
	if ( root == nullptr )
	{
		return;
	}

	if ( acm_get_bool( root, "visibleInEditor", true ) )
	{
		const char *name = acm_get_string( root, "name", nullptr );
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
			{
				project->icon = &folderIcon;
			}

			projects.emplace( project->internalName, project );
		}
	}

	acm_branch_destroy( root );
}

long forge::ProjectDialog::on_select_project( FXObject *, FXSelector, void * )
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

long forge::ProjectDialog::on_accept( FXObject *obj, FXSelector sel, void *ptr )
{
	// urgh, check if we have a valid project selected and if not,
	// that the user has entered *something*
	editorProject = ( Project * ) listBox->getItemData( listBox->getCurrentItem() );
	if ( ( editorProject == nullptr ) && ( projectNameField->getText() != defaultName ) )
	{
		PLPath folderName;
		editorProject = create_project(
		        std::string( projectNameField->getText().text() ),
		        PlSetupPath( folderName, true, "%s", projectNameField->getText().text() ) );
	}

	open_project( editorProject->internalName.c_str() );

	return onCmdAccept( obj, sel, ptr );
}
