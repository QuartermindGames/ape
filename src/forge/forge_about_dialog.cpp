// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include "forge_about_dialog.h"

FXIcon *forge::AboutDialog::editorIcon    = nullptr;
FXIcon *forge::AboutDialog::developerIcon = nullptr;

forge::AboutDialog::AboutDialog( FX::FXWindow *parent )
    : FXDialogBox( parent, FXString( "About " ) + FORGE_APP_TITLE )
{
	auto *infoFrame = new FXVerticalFrame( this, LAYOUT_CENTER_X | JUSTIFY_CENTER_X );

	new FXLabel( infoFrame, FXString( FORGE_APP_TITLE ) + " (" FORGE_APP_VERSION ")\n" SS_COM_COPYRIGHT "\n"
	                                                      "Editor environment for use with APE Tech.",
	             nullptr,
	             LAYOUT_LEFT | LAYOUT_SIDE_LEFT | JUSTIFY_LEFT );

	if ( editorIcon == nullptr )
		editorIcon = forge::load_fx_icon( FXApp::instance(), "resources/logo_editor.gif" );
	if ( developerIcon == nullptr )
		developerIcon = forge::load_fx_icon( FXApp::instance(), "resources/logo_developer.gif" );

	auto hframe = new FXHorizontalFrame( infoFrame, LAYOUT_FILL_X );
	new FXLabel( hframe, FXString::null, editorIcon, LAYOUT_CENTER_X | JUSTIFY_CENTER_X | JUSTIFY_CENTER_Y | LAYOUT_CENTER_Y );
	new FXLabel( hframe, FXString::null, developerIcon, LAYOUT_CENTER_X | JUSTIFY_CENTER_X | JUSTIFY_CENTER_Y | LAYOUT_CENTER_Y );

	new FXLabel( infoFrame, "This software uses the FOX Toolkit (http://www.fox-toolkit.org)." );

	new FXHorizontalSeparator( infoFrame );

	new FXButton( infoFrame, "Close", nullptr, this, ID_ACCEPT, BUTTON_NORMAL | LAYOUT_RIGHT );

	setWidth( 450 );
}
