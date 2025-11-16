// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Primary source file for QM2.
// Author:  Mark E. Sowden

#include "gateway.h"

static bool qm2_request_handler( ApeGameInterfaceRequest request, void *user )
{
	return true;
}

const ApeGameInterfaceImport *ape_game_get_interface()
{
	static ApeGameInterfaceImport interface = {
	        .version         = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = GAME_NET_PROTOCOL_VERSION,
	        .identifier      = "qm2",

	        .requestCallbackMethod = qm2_request_handler,
	};

	return &interface;
}
