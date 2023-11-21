// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_frame_console.h"

FXDEFMAP( os::editor::ConsoleFrame )
consoleFrameMap[] = {
        FXMAPFUNC( SEL_COMMAND, os::editor::ConsoleFrame::ID_SUBMIT, os::editor::ConsoleFrame::SubmitCommand ),
        FXMAPFUNC( SEL_KEYPRESS, os::editor::ConsoleFrame::ID_SUBMIT_FIELD, os::editor::ConsoleFrame::SubmitKey ),
};

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

	auto *submissionFrame = new FXHorizontalFrame( this, FRAME_NORMAL | LAYOUT_FILL_X );
	submitField           = new FXTextField( submissionFrame, 1, this, ID_SUBMIT_FIELD, FRAME_NORMAL | LAYOUT_FILL_X );
	submitButton          = new FXButton( submissionFrame, "Submit", nullptr, this, ID_SUBMIT );
}

os::editor::ConsoleFrame::~ConsoleFrame() = default;

void os::editor::ConsoleFrame::PushMessage( int, const char *msg, const PLColour & )
{
	logField->appendText( msg );
}

long os::editor::ConsoleFrame::SubmitCommand( FXObject *, FXSelector, void * )
{
	FXString command = submitField->getText();
	if ( command.empty() )
	{
		return false;
	}

	PlParseConsoleString( command.text() );

	submitField->setText( "" );
	return true;
}

long os::editor::ConsoleFrame::SubmitKey( FXObject *obj, FXSelector sel, void *ptr )
{
	const FXEvent *event = ( FXEvent * ) ptr;
	if ( event->code == FX::KEY_Return )
	{
		return SubmitCommand( obj, sel, ptr );
	}

	return false;
}
