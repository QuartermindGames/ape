// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

void ape_initialize_client_( void );
void ape_shutdown_client_( void );

void ape_render_frame_( ApeViewport *viewport );

void ape_tick_client_( void );

void ape_initiate_client_connection_( const char *ip, unsigned short port );
void ape_client_disconnect_( void );

#define CLIENT_PRINT( FORMAT, ... ) \
	Console_Print( APE_LOG_CLIENT_INFORMATION, FORMAT, ##__VA_ARGS__ )
#define CLIENT_PRINT_WARNING( FORMAT, ... ) \
	Console_Print( APE_LOG_CLIENT_WARNING, FORMAT, ##__VA_ARGS__ )
#define YINENGINE_CLIENT_PRINT_ERROR( FORMAT, ... ) \
	Console_Print( YINENGINE_LOG_CLIENT_ERROR, FORMAT, ##__VA_ARGS__ )

PL_EXTERN_C_END
