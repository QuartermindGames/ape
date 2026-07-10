// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmos/public/qm_os.h"
#include "qmos/public/qm_os_memory.h"
#include "qmos/public/qm_os_linked_list.h"

#include "qmmath/public/qm_math_vector.h"
#include "qmmath/public/qm_math_quaternion.h"
#include "qmmath/public/qm_math_colour.h"

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_package.h>
#include <plcore/pl_array_vector.h>
#include <plcore/pl_timer.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_polygon.h>

#include <acm/acm.h>

#include <assert.h>

#include "aux/public/aux.h"
#include "aux/public/aux_math.h"
#include "aux/public/aux_log.h"

#include <yin/core.h>

#define ENGINE_NAME     "ApeTech"
#define ENGINE_APP_NAME "ape"

#define VERSION_MAJOR 0
#define VERSION_MINOR 6
#define VERSION_PATCH 0

#define ENGINE_VERSION_STR           \
	QM_OS_TO_STRING( VERSION_MAJOR ) \
	"." QM_OS_TO_STRING( VERSION_MINOR ) "." QM_OS_TO_STRING( VERSION_PATCH )

#define S_STRCAT( DST, SOURCE ) strncat( ( DST ), ( SOURCE ), sizeof( ( DST ) ) - strlen( ( DST ) ) - 1 )

PL_EXTERN_C

void apeUpdateProfilerGraphs( void );

#include "ape_scheduler.h"
#include "memory/memory.h"

struct AcmBranch *ape_get_config_( void );
struct AcmBranch *ape_get_user_config_( void );

/****************************************
 * CONSOLE
 ****************************************/

typedef enum ApeConsoleLogLevel
{
	APE_LOG_ERROR,
	APE_LOG_WARNING,
	APE_LOG_INFORMATION,
	ACL_LOG_DEBUG,

	APE_LOG_VERBOSE,

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
	char            buffer[ CONSOLE_BUFFER_MAX_LENGTH ];
	QmMathColour4ub colour;
} ApeConsoleLine;

typedef struct ApeConsoleOutput
{
	ApeConsoleLine lines[ CONSOLE_BUFFER_MAX_LINES ];
	unsigned int   numLines;
	unsigned int   scrollPos;
} ApeConsoleOutput;

ApeConsoleOutput *apeGetConsoleOutput( void );

void ape_initialize_console_( void );
void ape_shutdown_console_( void );

#include "console/console.h"

typedef struct ApeConfig
{
	struct
	{
		bool wireframe;
		bool useStencilShadowVolumes;
		bool showShadowWireframe;
		bool showFps;
		bool showFaceBounds;
		bool showFaceNormals;
		bool showLights;
		bool forceShadows;
		bool skipRoomCull;
		bool showSelectionBuffer;

		float fogNearOverride;
		float fogFarOverride;

		float framebufferScale;
		float maxLightDistance;

		int lightJitterSamples;
		int msaaSamples;
	} renderer;

	struct
	{
		bool skipDraw;
		bool skipPortals;

		bool showPortals;
		bool showAllRooms;
		bool showNodeVolumes;

		bool sortLights;

		QmMathVector3f gravityModifier;
	} world;

	bool editor;
} ApeConfig;

extern ApeConfig ape_config_;

void ape_initialize_game_( void );
void ape_shutdown_game_( void );

void ape_tick_game_server_( double delta );

PL_EXTERN_C_END
