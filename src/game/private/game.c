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
	const YNCoreEntityComponentCallbackTable *EntityComponent_Transform_GetCallbackTable( void );
	ogeEntityManager_RegisterComponent( "transform", EntityComponent_Transform_GetCallbackTable() );

	const YNCoreEntityComponentCallbackTable *Game_Component_Movement_GetCallbackTable( void );
	ogeEntityManager_RegisterComponent( "movement", Game_Component_Movement_GetCallbackTable() );

	const YNCoreEntityComponentCallbackTable *Game_Component_Camera_GetCallbackTable( void );
	ogeEntityManager_RegisterComponent( "camera", Game_Component_Camera_GetCallbackTable() );

	const YNCoreEntityComponentCallbackTable *EntityComponent_Mesh_GetCallbackTable( void );
	ogeEntityManager_RegisterComponent( "mesh", EntityComponent_Mesh_GetCallbackTable() );
}

void Game_PlayerConnected( const char *name, unsigned int id )
{
}

void Game_PlayerDisconnected( unsigned int id )
{
}

static GameDifficulty gameDifficulty         = GAME_DIFFICULTY_NORMAL;
static GameConnectionType gameConnectionType = GAME_CONNECTION_LOCAL;

void Game_SetDifficultyMode( const GameDifficulty difficulty ) { gameDifficulty = difficulty; }
GameDifficulty Game_GetDifficultyMode( void ) { return gameDifficulty; }

void Game_SetConnection( const GameConnectionType connectionType )
{
	if ( gameConnectionType != GAME_CONNECTION_NONE )
	{
	}

	gameConnectionType = connectionType;
}

GameConnectionType Game_GetConnectionType( void )
{
	return gameConnectionType;
}
