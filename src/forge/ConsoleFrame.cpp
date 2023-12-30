// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "ConsoleFrame.h"

FXDEFMAP( ss::forge::ConsoleFrame )
consoleFrameMap[] = {
        FXMAPFUNC( SEL_COMMAND, ss::forge::ConsoleFrame::ID_SUBMIT, ss::forge::ConsoleFrame::submit_command ),
        FXMAPFUNC( SEL_KEYPRESS, ss::forge::ConsoleFrame::ID_SUBMIT_FIELD, ss::forge::ConsoleFrame::submit_key ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::ConsoleFrame::ID_CLEAR, ss::forge::ConsoleFrame::clear_command ),
};

FXIMPLEMENT( ss::forge::ConsoleFrame, FXVerticalFrame, consoleFrameMap, ARRAYNUMBER( consoleFrameMap ) )

ss::forge::ConsoleFrame::ConsoleFrame( FXComposite *composite )
    : FXVerticalFrame( composite, FRAME_NORMAL | LAYOUT_FILL )
{
	setPadBottom( 0 );
	setPadTop( 0 );
	setPadLeft( 0 );
	setPadRight( 0 );

	logField = new FXText( this, nullptr, 0, TEXT_READONLY | TEXT_AUTOSCROLL | LAYOUT_FILL );
	logField->setEditable( false );

	auto *submissionFrame = new FXHorizontalFrame( this, FRAME_NORMAL | LAYOUT_FILL_X );
	new FXButton( submissionFrame, "", ss::forge::load_fx_icon( FXApp::instance(), "resources/trash.gif" ), this, ID_CLEAR );
	submitField = new FXTextField( submissionFrame, 1, this, ID_SUBMIT_FIELD, FRAME_NORMAL | LAYOUT_FILL_X );
	submitButton = new FXButton( submissionFrame, "Submit", nullptr, this, ID_SUBMIT );
}

ss::forge::ConsoleFrame::~ConsoleFrame() = default;

void ss::forge::ConsoleFrame::push_message( int level, const char *msg, const PLColour &colour )
{
	logField->appendText( msg, ( int ) strlen( msg ), true );
	logField->makePositionVisible( logField->getBottomLine() );// given autoscroll doesn't work...
	logField->layout();
	logField->update();
}

long ss::forge::ConsoleFrame::submit_command( FXObject *, FXSelector, void * )
{
	FXString command = submitField->getText();
	if ( command.empty() )
		return false;

	PlParseConsoleString( command.text() );

	submitField->setText( "" );
	return true;
}

long ss::forge::ConsoleFrame::submit_key( FXObject *obj, FXSelector sel, void *ptr )
{
	const FXEvent *event = ( FXEvent * ) ptr;
	if ( event->code == FX::KEY_Return )
		return submit_command( obj, sel, ptr );

	return false;
}

long ss::forge::ConsoleFrame::clear_command( FXObject *, FXSelector, void * )
{
	logField->setText( "" );
	return 0;
}
