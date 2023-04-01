// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_dialog_project.h"
#include "3rdparty/fox/src/icons.h"

std::map< std::string, os::editor::Project * > os::editor::ProjectDialog::projects;

os::editor::ProjectDialog::ProjectDialog( FX::FXWindow *parent )
    : FXDialogBox( parent, "Project Setup" )
{
	setWidth( 256 );

	if ( projects.empty() )
	{
		// do a quick scan to see what projects are available
		PlScanDirectory( os::editor::cachedPaths[ os::editor::PATH_PROJECTS ], "n", RegisterProjectCallback, false, this );
	}

	FXVerticalFrame *vf = new FXVerticalFrame( this, LAYOUT_FILL );

	listBox = new FXListBox( vf, nullptr, 0, FRAME_SUNKEN | FRAME_THICK | LISTBOX_NORMAL | LAYOUT_FILL_X );
	for ( auto &i : projects )
	{
		listBox->appendItem( FXString( i.second->name.c_str() ) + " (" + i.second->rootDir.c_str() + ")", nullptr, ( void * ) &i );
	}
	static FXGIFIcon icon( getApp(), FX::foldernew );
	listBox->appendItem( "Create new project", &icon, nullptr );

	new FXHorizontalSeparator( vf );

	FXHorizontalFrame *hf = new FXHorizontalFrame( vf, LAYOUT_FILL | LAYOUT_RIGHT );
	new FXButton( hf, "OK" );
}

void os::editor::ProjectDialog::RegisterProjectCallback( const char *path, void *data )
{
	YNNodeBranch *root = YnNode_LoadFile( path, "project" );
	if ( root == nullptr )
	{
		return;
	}

	const char *name = YnNode_GetStringByName( root, "name", nullptr );
	if ( name == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), 0,
		                       "Warning",
		                       "Encountered a project without a name!\n"
		                       "%s",
		                       path );
		return;
	}

	const char *dir = YnNode_GetStringByName( root, "rootDir", nullptr );
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
