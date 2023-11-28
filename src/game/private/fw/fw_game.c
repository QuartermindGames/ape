// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "fw_game.h"
#include "fw_terrain.h"

#include "menu/fw_menu.h"

FWGameState fwGameState;

static bool FW_Game_Initialize( void ) {
	PL_ZERO_( fwGameState );

	game_register_standard_entity_components();

	fw_menu_initialize();
	fw_terrain_initialize();

	return true;
}

static void FW_Game_Shutdown( void ) {
	//TODO: need mechanism for removing components

	fw_terrain_shutdown();
}

static void FW_Game_NewGame( const char *path ) {
}

static void FW_Game_SaveGame( const char *path ) {
	NdBranch *root = ndPushBackObject( NULL, "fwGameSave" );

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
}

static void FW_Game_Precache( void ) {
}

static void Tick( void ) {
	fw_menu_handle_input();

	fw_menu_tick();
}

static void FW_Game_Draw( void ) {
}

static void FW_Game_DrawMenu( const ApeViewport *viewport ) {
	fw_menu_draw( viewport );
}

static void spawn_level( ApeWorld *world ) {
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

static bool request_handler( GameModeRequest gameModeRequest, void *user ) {
	switch ( gameModeRequest ) {
		case GAMEMODE_REQUEST_INITIALIZE:
			return FW_Game_Initialize();
		case GAMEMODE_REQUEST_TICK:
			Tick();
			break;
		case GAMEMODE_REQUEST_HANDLE_INPUT:
			break;
		case GAMEMODE_REQUEST_SPAWN_LEVEL:
			spawn_level( ( ApeWorld * ) user );
			break;
		default:
			break;
	}

	return false;
}

const GameModeInterface *gameModeInterface;
const GameModeInterface *gameGetModeInterface( void ) {
	static GameModeInterface gameMode;
	PL_ZERO_( gameMode );

	gameMode.requestCallbackMethod = request_handler;

	return &gameMode;
}
