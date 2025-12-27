// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

void ape_initialize_client_( void );
void ape_shutdown_client_( void );

void ape_render_frame_( ApeViewport *viewport );

void ape_tick_client_( double delta );

void ape_initiate_client_connection_( const char *ip, unsigned short port );
void ape_client_disconnect_( void );

PL_EXTERN_C_END
