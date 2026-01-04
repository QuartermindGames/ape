// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

void ape_client_connect( const char *address, uint16_t port );
void ape_client_disconnect( void );

/**
 * Returns true if the current client is connected and validated.
 */
bool ape_is_client_connected( void );

bool ape_client_send( const void **buf, size_t *bufSizes, unsigned int numBuffers );

PL_EXTERN_C_END
