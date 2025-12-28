// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Primary source file for QM2.
// Author:  Mark E. Sowden

#include "gateway.h"

void gway_menu_initialize();
void gway_menu_draw( ApeViewport *viewport );

static bool gway_initialize()
{
	gway_menu_initialize();

	return true;
}

static bool gway_shutdown()
{
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
			gway_menu_draw( user );
			return true;
		}
	}

	return true;
}

const ApeGameInterfaceImport *ape_game_get_interface()
{
	static ApeGameInterfaceImport interface = {
	        .version         = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = GWAY_VERSION_PROTOCOL,
	        .identifier      = "qm2",

	        .requestCallbackMethod = gway_request_handler,
	};

	return &interface;
}
