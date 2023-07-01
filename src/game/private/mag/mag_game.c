// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for mag game project.

#include "mag_game.h"

static bool Initialize( void )
{
	return true;
}

static bool HandleRequest( GameModeRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		default:
			break;
		case GAMEMODE_REQUEST_INITIALIZE:
			return Initialize();
	}

	return false;
}

const GameModeInterface *gameGetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );
	gameMode.RequestCallbackMethod = HandleRequest;
	return &gameMode;
}
