// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor.h"
#include "editor_window_material.h"
#include "ViewportFrame.h"

FXDEFMAP( ss::forge::MaterialWindow )
materialWindowMap[] = {
        {} };

FXIMPLEMENT( ss::forge::MaterialWindow, FXTopWindow, materialWindowMap, ARRAYNUMBER( materialWindowMap ) )

ss::forge::MaterialWindow::MaterialWindow( FXApp *app, ApeMaterial *material )
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

	viewportFrame = new ViewportFrame( this, nullptr, SS_ARL_CAMERA_MODE_PERSPECTIVE );
}

ss::forge::MaterialWindow::~MaterialWindow()
{
	if ( material != nullptr )
		ss_arl_material_release( material );
}
