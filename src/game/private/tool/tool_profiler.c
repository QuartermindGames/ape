// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Profiling mode tool thingy.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_time.h"

#include "game_private.h"
#include "game_server.h"

#include "tool_profiler.h"

/////////////////////////////////////////////////////////////////////////////////////
// Profiling Mode
// This is a more specialised profiling mode that explicitly moves the camera to a
// set of points, taking samples and then spitting out the results before closing.
// It's a little gross, but hey ho.

constexpr uint8_t MAX_CAMERAS = 8;
constexpr uint8_t MAX_SAMPLES = 8;

struct
{
	bool isProfiling;

	ApeCamera   *cameras[ MAX_CAMERAS ];
	uint8_t      numCameras;
	unsigned int curCamera;
} state;

static void save_view_command(
        [[maybe_unused]] unsigned int argc,
        const char *const            *argv )
{
	const GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr ||
	     player->camera == nullptr ||
	     player->entity == nullptr )
	{
		return;
	}

	const ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( player->entity ) );
	if ( room == nullptr )
	{
		return;
	}

	const unsigned int index = strtoul( argv[ 1 ], nullptr, 10 );
	if ( index >= MAX_CAMERAS )
	{
		game_warning_( "Invalid camera slot (%u >= %u)!\n", index, MAX_CAMERAS );
		return;
	}

	const QmMathVector3f pos = ape_camera_get_position( player->camera );
	const QmMathVector3f ang = ape_camera_get_angles( player->camera );

	char *path = qm_os_string_alloc( "%s/camera_%u.dat", com_get_app_data_directory(), index );
	FILE *file = fopen( path, "w" );
	qm_os_memory_free( path );

	if ( file == nullptr )
	{
		game_warning_( "Failed to create camera file (%s)!\n", path );
		return;
	}

	fprintf( file, "%s %f %f %f %f %f %f",
	         ape_world_node_get_path( APE_WORLD_NODE( room ) ),
	         pos.x, pos.y, pos.z,
	         ang.x, ang.y, ang.z );

	fclose( file );

	game_print_( "Saved view %u!\n", index );
}

static ApeCamera *load_camera_view( const unsigned int index )
{
	const ApeWorld *world = game_get_current_world();
	if ( world == nullptr )
	{
		return nullptr;
	}

	char *path = qm_os_string_alloc( "%s/camera_%u.dat", com_get_app_data_directory(), index );
	FILE *file = fopen( path, "r" );
	qm_os_memory_free( path );

	if ( file == nullptr )
	{
		return nullptr;
	}

	QmMathVector3f pos = {};
	QmMathVector3f ang = {};

	char str[ 256 ];
	fscanf( file, "%s %f %f %f %f %f %f",
	        str,
	        &pos.x, &pos.y, &pos.z,
	        &ang.x, &ang.y, &ang.z );

	fclose( file );

	const ApeRoom *room = ape_world_get_room_by_path( world, str );
	if ( room == nullptr )
	{
		game_warning_( "Failed to fetch room (%s) by path!\n", str );
		return nullptr;
	}

	ApeCamera *camera = ape_create_camera( APE_WORLD_NODE( room ), "prof_", &pos, &ang, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
	if ( camera == nullptr )
	{
		game_warning_( "Failed to create camera for profiling!\n" );
		return nullptr;
	}

	return camera;
}

static void start_command(
        [[maybe_unused]] unsigned int       argc,
        [[maybe_unused]] const char *const *argv )
{
	// collect up all of the cameras we've got available
	for ( uint8_t i = 0; i < MAX_CAMERAS; ++i )
	{
		ApeCamera *camera = load_camera_view( i );
		if ( camera == nullptr )
		{
			break;
		}

		state.cameras[ state.numCameras++ ] = camera;
	}

	state.isProfiling = true;
	state.curCamera   = 0;
}

void game_tool_profiler_initialize_()
{
	ape_console_cmd_register( "game_tool_profiler_save_view", "Save the current camera view.", 1, save_view_command );
	ape_console_cmd_register( "game_tool_profiler_start", "Starts the profiling tool.", 0, start_command );
}

bool game_tool_profiler_is_active_()
{
	return state.isProfiling;
}

void game_tool_profiler_draw_( const ApeViewport *viewport )
{
	if ( state.curCamera >= state.numCameras )
	{
		QM_OS_ZERO_( state );
		return;
	}

	ApeCamera *camera = state.cameras[ state.curCamera ];
	if ( camera == nullptr )
	{
		QM_OS_ZERO_( state );
		return;
	}

	double totalAvg = 0.0;
	for ( uint8_t i = 0; i < MAX_SAMPLES; ++i )
	{
		const double startTime = qm_os_time_get_seconds() * 1000.0;

		ape_camera_make_active( camera );
		ape_camera_draw_perspective( camera, viewport );

		const double endTime   = qm_os_time_get_seconds() * 1000.0;
		const double timeTaken = endTime - startTime;

		totalAvg += timeTaken;

		game_print_( "Camera %u, sample %u: %lf ms\n", state.curCamera, i, timeTaken );
	}

	totalAvg = totalAvg / MAX_SAMPLES;
	game_print_( "Avg. time %lf ms\n", totalAvg );

	ape_world_node_destroy( APE_WORLD_NODE( camera ) );
	state.cameras[ state.curCamera ] = nullptr;
	state.curCamera++;
}
