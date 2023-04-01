// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_dialog_about.h"

os::editor::AboutDialog::AboutDialog( FX::FXWindow *parent )
: FXDialogBox( parent, "About " EDITOR_APP_TITLE )
{
	FXVerticalFrame *infoFrame = new FXVerticalFrame( this );

	new FXLabel( infoFrame, EDITOR_APP_TITLE " (" EDITOR_APP_VERSION ")" );
	new FXLabel( infoFrame, "Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>" );
	new FXHorizontalSeparator( infoFrame );

	new FXButton( infoFrame, "OK", nullptr, this, ID_ACCEPT );
}
