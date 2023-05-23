// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Main file for demo game project.

#include "demo_game.h"

static const char *baseVppPaths[] = {
        "RF_PS2.VPP",
};
static PLFileSystemMount *baseVpps[ PL_ARRAY_ELEMENTS( baseVppPaths ) ];

static void InitializeDemoGame( void )
{
	for ( uint32_t i = 0; i < PL_ARRAY_ELEMENTS( baseVppPaths ); ++i )
	{
		baseVpps[ i ] = PlMountLocation( baseVppPaths[ i ] );
		if ( baseVpps[ i ] == NULL )
		{
			Game_Warning( "Failed to mount base VPP: %s\n", PlGetError() );
		}
	}

	Game_RegisterStandardEntityComponents();

	PlParseConsoleString( "world test" );

	OgeCamera *camera = ogeCamera_Create( "test", &pl_vecOrigin3, &pl_vecOrigin3 );
	ogeMakeCameraActive( camera );
}

static void ShutdownDemoGame( void )
{
	for ( uint32_t i = 0; i < PL_ARRAY_ELEMENTS( baseVppPaths ); ++i )
	{
		PlClearMountedLocation( baseVpps[ i ] );
		baseVpps[ i ] = NULL;
	}
}

static void TickDemoGame( void )
{
}

static bool HandleRequest( GameModeRequest modeRequest, void *user )
{
	switch ( modeRequest )
	{
		case GAMEMODE_REQUEST_TICK:
			TickDemoGame();
			break;
		case GAMEMODE_REQUEST_HANDLEINPUT:
			break;
		case GAMEMODE_REQUEST_SPAWNWORLD:
			break;
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *Game_GetModeInterface( void )
{
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );

	gameMode.Initialize = InitializeDemoGame;
	gameMode.Shutdown   = ShutdownDemoGame;
	//gameMode.NewGame               = FW_Game_NewGame;
	//gameMode.SaveGame              = FW_Game_SaveGame;
	//gameMode.RestoreGame           = FW_Game_RestoreGame;
	//gameMode.Precache              = FW_Game_Precache;
	//gameMode.Draw                  = FW_Game_Draw;
	//gameMode.DrawMenu              = FW_Game_DrawMenu;
	gameMode.RequestCallbackMethod = HandleRequest;

	return &gameMode;
}
