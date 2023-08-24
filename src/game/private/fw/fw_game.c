// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "fw_game.h"
#include "fw_terrain.h"

#include "menu/fw_menu.h"

FWGameState fwGameState;

static void FW_Game_Initialize( void ) {
	PL_ZERO_( fwGameState );

	Game_RegisterStandardEntityComponents();

	// Register our FW specific components
	ogeEntityManager_RegisterComponent( "fw.character", NULL );
	ogeEntityManager_RegisterComponent( "fw.projectile", NULL );
	ogeEntityManager_RegisterComponent( "fw.weapon", NULL );

	FW_Menu_Initialize();
	FW_Terrain_Initialize();
}

static void FW_Game_Shutdown( void ) {
	//TODO: need mechanism for removing components

	FW_Terrain_Shutdown();
}

static void FW_Game_NewGame( const char *path ) {
}

static void FW_Game_SaveGame( const char *path ) {
	NdBranch *root = ndPushBackObject( NULL, "fwGameSave" );

	// Save entity data
	YnCore_EntityManager_Save( ndPushBackObject( root, "entityData" ) );

	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) ) {
		Game_Warning( "Failed to write save (%s): %s\n", path, ndGetErrorMessage() );
		return;
	}

	ndDestroyBranch( root );
}

static void FW_Game_RestoreGame( const char *path ) {
	NdBranch *root = ndLoadFile( path, "fwGameSave" );
	if ( root == NULL ) {
		Game_Warning( "Failed to load game save (%s): %s\n", path, ndGetErrorMessage() );
		return;
	}

	NdBranch *entityNode = ndGetChildByName( root, "entityData" );
	if ( entityNode != NULL ) {
		ogeEntityManager_Restore( NULL );
	}

	ndDestroyBranch( entityNode );
}

static void FW_Game_Precache( void ) {
}

static void Tick( void ) {
	FW_Menu_HandleInput();

	FW_Menu_Tick();
}

static void FW_Game_Draw( void ) {
}

static void FW_Game_DrawMenu( const ApeViewport *viewport ) {
	FW_Menu_Draw( viewport );
}

static void SpawnWorld( ApeWorld *world ) {
#if 0
	NdBranch *propertyNode;
	if ( ( propertyNode = ogeWorld_GetProperty( world, "heightmap" ) ) != NULL )
	{
		PLPath path;
		if ( ndGetStr( propertyNode, path, sizeof( path ) ) == ND_ERROR_SUCCESS )
		{

		}
		else
		{
			Game_Warning( "Invalid heightmap property encountered for world (%s)!\n", YnCore_World_GetPath( world ) );
		}
	}
	else
	{
		Game_Warning( "No heightmap provided for world (%s)!\n", YnCore_World_GetPath( world ) );
	}

	if ( ( propertyNode = ogeWorld_GetProperty( world, "waterLevel" ) ) != NULL )
	{
		ndGetF32( propertyNode, &fwGameState.simState.waterHeight );
	}
#endif
}

static bool FW_Game_RequestHandler( GameModeRequest gameModeRequest, void *user ) {
	switch ( gameModeRequest ) {
		case GAMEMODE_REQUEST_TICK:
			Tick();
			break;
		case GAMEMODE_REQUEST_HANDLEINPUT:
			break;
		case GAMEMODE_REQUEST_SPAWNWORLD:
			SpawnWorld( ( ApeWorld * ) user );
			break;
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *Game_GetModeInterface( void ) {
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );

	gameMode.Initialize = FW_Game_Initialize;
	gameMode.Shutdown = FW_Game_Shutdown;
	gameMode.NewGame = FW_Game_NewGame;
	gameMode.SaveGame = FW_Game_SaveGame;
	gameMode.RestoreGame = FW_Game_RestoreGame;
	gameMode.Precache = FW_Game_Precache;
	gameMode.Draw = FW_Game_Draw;
	gameMode.DrawMenu = FW_Game_DrawMenu;
	gameMode.RequestCallbackMethod = FW_Game_RequestHandler;

	return &gameMode;
}
