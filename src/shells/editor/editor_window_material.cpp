// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor.h"
#include "editor_window_material.h"
#include "editor_frame_viewport.h"

FXDEFMAP( os::editor::MaterialWindow )
materialWindowMap[] = {
        {} };

FXIMPLEMENT( os::editor::MaterialWindow, FXTopWindow, materialWindowMap, ARRAYNUMBER( materialWindowMap ) )

os::editor::MaterialWindow::MaterialWindow( FXApp *app, YNCoreMaterial *material )
    : FXTopWindow(
              app,
              "Material Editor",
              nullptr,
              nullptr,
              0, 0,
              500, 500,
              0, 0, 0, 0, 0, 0, 0 )
{
	this->material = material;

	viewportFrame = new ViewportFrame( this, nullptr, YN_CORE_CAMERA_MODE_PERSPECTIVE );
}

os::editor::MaterialWindow::~MaterialWindow()
{
	if ( material != nullptr )
	{
		YnCore_Material_Release( material );
	}
}
