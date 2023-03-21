// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "ConsoleFrame.h"

FXDEFMAP( os::editor::ConsoleFrame )
consoleFrameMap[] = { {}};

FXIMPLEMENT( os::editor::ConsoleFrame, FXVerticalFrame, consoleFrameMap, ARRAYNUMBER( consoleFrameMap ) )

os::editor::ConsoleFrame::ConsoleFrame( FXComposite *composite )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL | TEXT_AUTOSCROLL )
{
	setPadBottom( 0 );
	setPadTop( 0 );
	setPadLeft( 0 );
	setPadRight( 0 );

	logField = new FXText( this, nullptr, 0, LAYOUT_FILL );
	logField->setEditable( false );
	//logField->setBackColor( FXRGB( 0, 0, 0 ) );
	//logField->setTextColor( FXRGB( 255, 255, 255 ) );

	auto *submissionFrame = new FXHorizontalFrame( this, LAYOUT_FILL_X );
	submitField                        = new FXTextField( submissionFrame, 1, nullptr, 0, FRAME_NORMAL | LAYOUT_FILL_X );
	submitButton                       = new FXButton( submissionFrame, "Submit" );
}

os::editor::ConsoleFrame::~ConsoleFrame() = default;

void os::editor::ConsoleFrame::PushMessage( int level, const char *msg, const PLColour &colour )
{
	PL_UNUSEDVAR( level );
	PL_UNUSEDVAR( colour );
	logField->appendText( msg );
}
