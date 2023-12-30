// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_face_inspector.h"
#include "MainWindow.h"

FXIMPLEMENT( EditorFaceInspector, FXDialogBox, NULL, 0 )

EditorFaceInspector::EditorFaceInspector( EditorMainWindow *parent )
    : FXDialogBox( parent, "Face Inspector", DECOR_TITLE | DECOR_BORDER )
{
	FXVerticalFrame   *vertical = new FXVerticalFrame( this, LAYOUT_SIDE_TOP | LAYOUT_FILL_X | LAYOUT_FILL_Y );
	FXHorizontalFrame *horizontal = new FXHorizontalFrame( vertical, LAYOUT_FILL_X | LAYOUT_FILL_Y );

	{
		FXVerticalFrame *m = new FXVerticalFrame( horizontal );
		FXHorizontalFrame *j = new FXHorizontalFrame( m );
		new FXLabel( j, "Scale X:" );

		new FXLabel( m, "Scale Y:" );
		new FXLabel( m, "Material:" );

		new FXRealSlider( m );
		new FXRealSlider( m );

		new FXHorizontalSeparator( m );
		new FXCheckButton( m, "Double sided" );
		new FXCheckButton( m, "Portal" );
		new FXCheckButton( m, "Mirror" );
		new FXHorizontalSeparator( m );
		new FXCheckButton( m, "Fullbright" );
	}

	show();
}
