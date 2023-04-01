// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_dialog_project.h"

os::editor::ProjectDialog::ProjectDialog( FX::FXWindow *parent )
    : FXDialogBox( parent, "Project Setup" )
{
	// do a quick scan to see what projects are available
	PlScanDirectory( os::editor::cachedPaths[ os::editor::PATH_PROJECTS ], "cfg.n", RegisterProjectCallback, false, this );

}

void os::editor::ProjectDialog::RegisterProjectCallback( const char *path, void *data )
{
	YNNodeBranch *root = YnNode_LoadFile( path, "project" );
	if ( root == nullptr )
	{
		return;
	}

	ProjectDialog *projectDialog = ( ProjectDialog * ) data;

	YnNode_DestroyBranch( root );
}
