// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

void ss_acl_initialize_client_( void );
void ss_acl_shutdown_client_( void );

void ss_arl_render_frame_( SSArlViewport *viewport );

void ss_acl_tick_client_( void );

void ss_acl_initiate_client_connection_( const char *ip, unsigned short port );
void ss_acl_client_disconnect_( void );

#define CLIENT_PRINT( FORMAT, ... ) \
	Console_Print( APE_LOG_CLIENT_INFORMATION, FORMAT, ##__VA_ARGS__ )
#define CLIENT_PRINT_WARNING( FORMAT, ... ) \
	Console_Print( APE_LOG_CLIENT_WARNING, FORMAT, ##__VA_ARGS__ )
#define YINENGINE_CLIENT_PRINT_ERROR( FORMAT, ... ) \
	Console_Print( YINENGINE_LOG_CLIENT_ERROR, FORMAT, ##__VA_ARGS__ )

PL_EXTERN_C_END
