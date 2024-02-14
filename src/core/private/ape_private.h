// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>
#include <plcore/pl_linkedlist.h>
#include <plcore/pl_array_vector.h>
#include <plcore/pl_timer.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_camera.h>
#include <plgraphics/plg_polygon.h>

#include <assert.h>

#include "common.h"
#include "common/common_math.h"

#include <yin/core.h>
#include <yin/node.h>

#define ENGINE_NAME     "APE Tech"
#define ENGINE_APP_NAME "ape"

#define VERSION_MAJOR    0
#define VERSION_MINOR    4
#define VERSION_PATCH    0
#define VERSION_CODENAME "Rutilus"

PL_EXTERN_C

#define ENGINE_VERSION_STR       \
	PL_TOSTRING( VERSION_MAJOR ) \
	"." PL_TOSTRING( VERSION_MINOR ) "." PL_TOSTRING( VERSION_PATCH ) " (" VERSION_CODENAME ")"

void apeUpdateProfilerGraphs( void );

#include "ape_scheduler.h"
#include "ape_memory_manager.h"

/****************************************
 * CONSOLE
 ****************************************/

typedef enum ApeConsoleLogLevel
{
	APE_LOG_ERROR,
	APE_LOG_WARNING,
	APE_LOG_INFORMATION,
	ACL_LOG_DEBUG,

	APE_LOG_CLIENT_ERROR,
	APE_LOG_CLIENT_WARNING,
	APE_LOG_CLIENT_INFORMATION,

	APE_LOG_SERVER_ERROR,
	APE_LOG_SERVER_WARNING,
	APE_LOG_SERVER_INFORMATION,

	APE_LOG_LEVELS
} ApeConsoleLogLevel;

#define CONSOLE_BUFFER_MAX_LENGTH 256
#define CONSOLE_BUFFER_MAX_LINES  2048

typedef struct ApeConsoleLine
{
	char buffer[ CONSOLE_BUFFER_MAX_LENGTH ];
	PLColour colour;
} ApeConsoleLine;

typedef struct ApeConsoleOutput
{
	ApeConsoleLine lines[ CONSOLE_BUFFER_MAX_LINES ];
	unsigned int numLines;
	unsigned int scrollPos;
} ApeConsoleOutput;

ApeConsoleOutput *apeGetConsoleOutput( void );

void ape_initialize_console_( void );
void ape_shutdown_console_( void );

int Console_GetLogLevel( ApeConsoleLogLevel level );
void Console_Print( ApeConsoleLogLevel level, const char *message, ... );

void ape_console_register_commands_( bool isDedicated );
void ape_console_register_variables_( bool isDedicated );

void ape_console_draw_( const ApeViewport *viewport );
void ape_console_register_cl_commands_( void );
void ape_console_register_cl_variables_( void );

#define PRINT( FORMAT, ... ) \
	Console_Print( APE_LOG_INFORMATION, FORMAT, ##__VA_ARGS__ )
#define PRINT_WARNING( FORMAT, ... ) \
	Console_Print( APE_LOG_WARNING, "WARNING: " FORMAT, ##__VA_ARGS__ )
#define PRINT_ERROR( FORMAT, ... )                                                   \
	{                                                                                \
		PlLogMessage( Console_GetLogLevel( APE_LOG_ERROR ), FORMAT, ##__VA_ARGS__ ); \
		abort();                                                                     \
	}

#if !defined( NDEBUG )
#	define PRINT_DEBUG( FORMAT, ... ) PlLogWFunction( Console_GetLogLevel( ACL_LOG_DEBUG ), FORMAT, ##__VA_ARGS__ )
#else
#	define PRINT_DEBUG( FORMAT, ... )
#endif

typedef struct ApeConfig
{
	struct
	{
		bool wireframe;
		bool useStencilShadowVolumes;
		bool showShadowWireframe;
		bool showFps;
		bool showFaceBounds;
		bool showLights;
		bool forceShadows;
		bool skipRoomCull;
		bool skipAmbience;

		float fogNearOverride;
		float fogFarOverride;

		float superSampling;
		float maxLightDistance;
	} renderer;

	struct
	{
		bool skipDraw;
		bool skipPortals;

		bool showPortals;
		bool showAllRooms;
		bool showRoomVolumes;

		bool sortLights;
	} world;

	bool editor;
} ApeConfig;

extern ApeConfig ape_config_;

void ape_initialize_game_( void );
void ape_shutdown_game_( void );

void ape_tick_game_( void );
void ape_spawn_world_( const char *worldPath );

PL_EXTERN_C_END
