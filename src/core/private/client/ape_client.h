// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PL_EXTERN_C

void apeInitializeClient_( void );
void apeShutdownClient_( void );
void ss_arl_render_frame( SS_Arl_Viewport *viewport );
void apeTickClient( void );

void apeInitiateClientConnection_( const char *ip, unsigned short port );
void apeDisconnectClient_( void );

#define CLIENT_PRINT( FORMAT, ... ) \
	Console_Print( APE_LOG_CLIENT_INFORMATION, FORMAT, ##__VA_ARGS__ )
#define CLIENT_PRINT_WARNING( FORMAT, ... ) \
	Console_Print( APE_LOG_CLIENT_WARNING, FORMAT, ##__VA_ARGS__ )
#define YINENGINE_CLIENT_PRINT_ERROR( FORMAT, ... ) \
	Console_Print( YINENGINE_LOG_CLIENT_ERROR, FORMAT, ##__VA_ARGS__ )

PL_EXTERN_C_END
