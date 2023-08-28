// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "game_private.h"

#include "game_component_mesh.h"
#include "game_component_test.h"

int globalGameLog;
int globalGameDebugLog;
int globalGameWarningLog;
int globalGameErrorLog;

#if 0
void Game_Display( void )
{
}
#endif

void gameRegisterStandardEntityComponents( void ) {
	const ApeEntityComponentCallbackTable *EntityComponent_Transform_GetCallbackTable( void );
	const ApeEntityComponentCallbackTable *Game_Component_Movement_GetCallbackTable( void );
	const ApeEntityComponentCallbackTable *Game_Component_Camera_GetCallbackTable( void );

	apeRegisterEntityComponent( "transform", EntityComponent_Transform_GetCallbackTable() );
	apeRegisterEntityComponent( "movement", Game_Component_Movement_GetCallbackTable() );
	apeRegisterEntityComponent( "camera", Game_Component_Camera_GetCallbackTable() );
	apeRegisterEntityComponent( "mesh", gameMeshComponentCallbackTable() );
	apeRegisterEntityComponent( "test", gameTestComponentCallbackTable() );
}

void gamePlayerConnected( const char *name, unsigned int id ) {
	Game_Print( "Player %s (%u) has connected\n", name, id );
}

void gamePlayerDisconnected( unsigned int id ) {
	Game_Print( "Player (%u) has disconnected\n", id );
}

static GameDifficulty gameDifficulty = GAME_DIFFICULTY_NORMAL;
static GameConnectionType gameConnectionType = GAME_CONNECTION_LOCAL;

void gameSetDifficultyMode( GameDifficulty difficulty ) { gameDifficulty = difficulty; }
GameDifficulty gameGetDifficultyMode( void ) { return gameDifficulty; }

void Game_SetConnection( const GameConnectionType connectionType ) {
	if ( gameConnectionType != GAME_CONNECTION_NONE ) {
	}

	gameConnectionType = connectionType;
}

GameConnectionType gameGetConnectionType( void ) {
	return gameConnectionType;
}
