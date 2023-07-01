// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for mag game project.

#include "mag_game.h"

static bool HandleRequest( GameModeRequest modeRequest, void *user )
{
	switch( modeRequest )
	{

	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *Game_GetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );

	return &gameMode;
}
