/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "game_private.h"

int globalGameLog;
int globalGameDebugLog;
int globalGameWarningLog;
int globalGameErrorLog;

#if 0
void Game_Display( void )
{
}
#endif

void Game_RegisterStandardEntityComponents( void )
{
	const ApeEntityComponentCallbackTable *EntityComponent_Transform_GetCallbackTable( void );
	apeRegisterEntityComponent( "transform", EntityComponent_Transform_GetCallbackTable() );

	const ApeEntityComponentCallbackTable *Game_Component_Movement_GetCallbackTable( void );
	apeRegisterEntityComponent( "movement", Game_Component_Movement_GetCallbackTable() );

	const ApeEntityComponentCallbackTable *Game_Component_Camera_GetCallbackTable( void );
	apeRegisterEntityComponent( "camera", Game_Component_Camera_GetCallbackTable() );

	const ApeEntityComponentCallbackTable *EntityComponent_Mesh_GetCallbackTable( void );
	apeRegisterEntityComponent( "mesh", EntityComponent_Mesh_GetCallbackTable() );
}

void gamePlayerConnected( const char *name, unsigned int id )
{
}

void gamePlayerDisconnected( unsigned int id )
{
}

static GameDifficulty gameDifficulty         = GAME_DIFFICULTY_NORMAL;
static GameConnectionType gameConnectionType = GAME_CONNECTION_LOCAL;

void gameSetDifficultyMode( GameDifficulty difficulty ) { gameDifficulty = difficulty; }
GameDifficulty gameGetDifficultyMode( void ) { return gameDifficulty; }

void Game_SetConnection( const GameConnectionType connectionType )
{
	if ( gameConnectionType != GAME_CONNECTION_NONE )
	{
	}

	gameConnectionType = connectionType;
}

GameConnectionType gameGetConnectionType( void )
{
	return gameConnectionType;
}
