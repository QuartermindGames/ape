// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

typedef struct ApeServerClient ApeServerClient;

typedef enum ApeServerClientState
{
	APE_SERVER_CLIENT_STATE_DISCONNECTED,// has lost connection with the server
	APE_SERVER_CLIENT_STATE_VALIDATING,  // has connected but is pending validation
	APE_SERVER_CLIENT_STATE_REJECTED,    // client has been rejected and will be dropped
	APE_SERVER_CLIENT_STATE_ACCEPTED,    // is connected and validation was successful
} ApeServerClientState;

bool ape_server_send( ApeServerClient *clientHandle, const void **buf, size_t *bufSizes, unsigned int numBuffers );

const char *ape_server_get_client_name( ApeServerClient *client );

PL_EXTERN_C_END
