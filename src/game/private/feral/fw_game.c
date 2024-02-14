// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <yin/node.h>

#include "fw_game.h"
#include "fw_terrain.h"
#include "fw_building.h"

#include "menu/fw_menu.h"

FWGameState fwGameState;

static bool fw_initialize( void )
{
	PL_ZERO_( fwGameState );

	ss_game_register_standard_entity_components_();

	ape_register_entity_class( &fw_buildingClassDefinition );

	fw_menu_initialize();
	fw_terrain_initialize();

	return true;
}

static bool fw_shutdown( void )
{
	//TODO: need mechanism for removing components

	fw_terrain_shutdown();

	return true;
}

static bool fw_tick( void )
{
	fw_menu_handle_input();

	fw_menu_tick();

	return true;
}

static bool fw_draw( void )
{
	return false;
}

static bool fw_draw_menu( const ApeViewport *viewport )
{
	fw_menu_draw( viewport );

	return true;
}

static bool fw_spawn_world( ApeWorld *world )
{
	return true;
}

static bool request_handler( ApeGameInterfaceRequest gameModeRequest, void *user )
{
	switch ( gameModeRequest )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return fw_initialize();
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN:
			return fw_shutdown();
		case APE_GAME_INTERFACE_REQUEST_DRAW:
			return fw_draw();
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
			return fw_draw_menu( user );
		case APE_GAME_INTERFACE_REQUEST_TICK:
			return fw_tick();
		case APE_GAME_INTERFACE_REQUEST_HANDLE_INPUT:
			break;
		case APE_GAME_INTERFACE_REQUEST_SPAWN_WORLD:
			return fw_spawn_world( ( ApeWorld * ) user );
		default:
			break;
	}

	return false;
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode;
	PL_ZERO_( gameMode );

	gameMode.requestCallbackMethod = request_handler;

	return &gameMode;
}
