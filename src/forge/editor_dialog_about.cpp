// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_dialog_about.h"

FXIcon *ss::forge::AboutDialog::editorIcon = nullptr;
FXIcon *ss::forge::AboutDialog::developerIcon = nullptr;

ss::forge::AboutDialog::AboutDialog( FX::FXWindow *parent )
    : FXDialogBox( parent, "About " SS_FORGE_APP_TITLE )
{
	FXVerticalFrame *infoFrame = new FXVerticalFrame( this );

	new FXLabel( infoFrame, SS_FORGE_APP_TITLE " (" SS_FORGE_APP_VERSION ")\n"
	                                           "Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>\n"
	                                           "Editor environment for use with APE Tech.",
	             nullptr,
	             LAYOUT_LEFT | LAYOUT_SIDE_LEFT | JUSTIFY_LEFT );

	if ( editorIcon == nullptr )
		editorIcon = ss::forge::load_fx_icon( FXApp::instance(), "resources/logo_editor.gif" );
	if ( developerIcon == nullptr )
		developerIcon = ss::forge::load_fx_icon( FXApp::instance(), "resources/logo_developer.gif" );

	auto hframe = new FXHorizontalFrame( infoFrame, LAYOUT_FILL_X );
	new FXLabel( hframe, FXString::null, editorIcon, LAYOUT_CENTER_X | JUSTIFY_CENTER_X | JUSTIFY_CENTER_Y | LAYOUT_CENTER_Y );
	new FXLabel( hframe, FXString::null, developerIcon, LAYOUT_CENTER_X | JUSTIFY_CENTER_X | JUSTIFY_CENTER_Y | LAYOUT_CENTER_Y );

	new FXHorizontalSeparator( infoFrame );

	new FXButton( infoFrame, "Close", nullptr, this, ID_ACCEPT, BUTTON_NORMAL | LAYOUT_RIGHT );
}
