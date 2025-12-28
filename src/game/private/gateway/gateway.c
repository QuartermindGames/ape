// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Primary source file for QM2.
// Author:  Mark E. Sowden

#include "gateway.h"

#include "game_client.h"
#include "game_server.h"

#include "integrations/integrations.h"

void gway_menu_initialize_();
void gway_menu_draw_( ApeViewport *viewport );

void gway_hud_initialize_();
void gway_hud_shutdown_();
void gway_hud_draw_( ApeViewport *viewport );

static bool gway_initialize()
{
	gway_menu_initialize_();
	gway_hud_initialize_();

	return true;
}

static bool gway_shutdown()
{
	gway_hud_shutdown_();

	return true;
}

static bool gway_request_handler( ApeGameInterfaceRequest request, void *user )
{
	switch ( request )
	{
		default:
			break;
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return gway_initialize();
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN:
			return gway_shutdown();
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
		{
			gway_menu_draw_( user );
			gway_hud_draw_( user );
			return true;
		}
	}

	return true;
}

static bool server_client_validate( ApeServerClient *clientHandle )
{
	return game_server_client_validate_( clientHandle );
}

static void server_client_connected( ApeServerClient *clientHandle )
{
	game_server_client_connected_( clientHandle );
}

static void server_client_disconnected( ApeServerClient *clientHandle )
{
	game_server_client_disconnected_( clientHandle );
}

static void server_process_message( ApeServerClient *clientHandle, const void *buf, size_t bufSize )
{
	game_server_process_message_( clientHandle, buf, bufSize );
}

static void client_process_message( const void *buf, size_t bufSize )
{
	game_client_process_message_( buf, bufSize );
}

static void client_tick( double delta )
{
	game_integrations_discord_tick_();
}

const ApeGameInterfaceImport *ape_game_get_interface()
{
	static ApeGameInterfaceImport interface = {
	        .version         = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = GWAY_VERSION_PROTOCOL,
	        .identifier      = "qm2",

	        .requestCallbackMethod = gway_request_handler,

	        .serverClientValidate     = server_client_validate,
	        .serverClientConnected    = server_client_connected,
	        .serverClientDisconnected = server_client_disconnected,
	        .serverProcessMessage     = server_process_message,

	        .clientProcessMessage = client_process_message,
	        .clientTick           = client_tick,
	};

	return &interface;
}
