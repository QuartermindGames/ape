// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_dialog_project.h"

#include "3rdparty/fox/src/icons.h"

std::map< std::string, os::editor::Project * > os::editor::ProjectDialog::projects;

FXDEFMAP( os::editor::ProjectDialog )
projectDialogMap[] = {
        FXMAPFUNC( SEL_COMMAND, os::editor::ProjectDialog::ID_SELECT_PROJECT, os::editor::ProjectDialog::OnSelectProject ),
        FXMAPFUNC( SEL_COMMAND, os::editor::ProjectDialog::ID_ACCEPT, os::editor::ProjectDialog::OnAccept ),
};

FXIMPLEMENT( os::editor::ProjectDialog, FXDialogBox, projectDialogMap, ARRAYNUMBER( projectDialogMap ) )

os::editor::ProjectDialog::ProjectDialog( FX::FXWindow *parent )
    : FXDialogBox( parent, "Project Setup" )
{
	setWidth( baseWidth );

	if ( projects.empty() )
	{
		// do a quick scan to see what projects are available
		PlScanDirectory( os::editor::cachedPaths[ os::editor::PATH_PROJECTS ], "n", RegisterProjectCallback, false, this );
	}

	FXVerticalFrame *vf = new FXVerticalFrame( this, LAYOUT_FILL );

	listBox = new FXListBox( vf, this, ID_SELECT_PROJECT, FRAME_SUNKEN | FRAME_THICK | LISTBOX_NORMAL | LAYOUT_FILL_X );
	for ( auto &i : projects )
	{
		listBox->appendItem( FXString( i.second->name.c_str() ) + " (" + i.second->rootDir.c_str() + ")", nullptr, ( void * ) &i );
	}
	static FXGIFIcon icon( getApp(), FX::foldernew );
	listBox->appendItem( "Create new project", &icon, nullptr );

	projectNameField = new FXTextField( vf, 1, nullptr, 0, TEXTFIELD_NORMAL | LAYOUT_FILL_X );
	projectNameField->setText( defaultName );
	if ( !projects.empty() )
	{
		projectNameField->hide();
	}

	new FXHorizontalSeparator( vf );

	FXHorizontalFrame *hf = new FXHorizontalFrame( vf, LAYOUT_FILL | LAYOUT_RIGHT );
	new FXButton( hf, "Accept", nullptr, this, ID_ACCEPT, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
	new FXButton( hf, "Cancel", nullptr, this, ID_CANCEL, BUTTON_INITIAL | BUTTON_DEFAULT | FRAME_RAISED | FRAME_THICK | LAYOUT_TOP | LAYOUT_LEFT | LAYOUT_CENTER_X );
}

void os::editor::ProjectDialog::RegisterProjectCallback( const char *path, void *data )
{
	NdBranch *root = ndLoadFile( path, "project" );
	if ( root == nullptr )
	{
		return;
	}

	const char *name = ndGetStringByName( root, "name", nullptr );
	if ( name == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), 0,
		                       "Warning",
		                       "Encountered a project without a name!\n"
		                       "%s",
		                       path );
		return;
	}

	const char *dir = ndGetStringByName( root, "rootDir", nullptr );
	if ( dir == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), 0,
		                       "Warning",
		                       "Encountered a project without a root directory!\n"
		                       "%s",
		                       path );
		return;
	}

	Project *project = new Project( name );
	project->config  = root;
	project->rootDir = dir;

	projects.emplace( name, project );
}

long os::editor::ProjectDialog::OnSelectProject( FXObject *, FXSelector, void * )
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

long os::editor::ProjectDialog::OnAccept( FXObject *obj, FXSelector sel, void *ptr )
{
	// urgh, check if we have a valid project selected and if not,
	// that the user has entered *something*
	editor::editorProject = ( Project * ) listBox->getItemData( listBox->getCurrentItem() );
	if ( ( editor::editorProject == nullptr ) && ( projectNameField->getText() != defaultName ) )
	{
		PLPath folderName;
		editor::editorProject = editor::CreateProject(
		        std::string( projectNameField->getText().text() ),
		        PlSetupPath( folderName, true, "%s", projectNameField->getText().text() ) );
	}

	return FXDialogBox::onCmdAccept( obj, sel, ptr );
}
