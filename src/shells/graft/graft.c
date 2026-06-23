// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Graft editor.
// Author:  Mark E. Sowden

#include "graft.h"

#include "plcore/pl_filesystem.h"

#include "plgraphics/plg.h"
#include "plgraphics/plg_driver_interface.h"

typedef struct GraftWindow
{
	SDL_Window *system;
	void       *user;
} GraftWindow;

SDL_GLContext glContext;

static bool setup_display()
{
	PlgInitializeGraphics();

	// attempt to fetch the driver directly from the executable location if possible
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) != NULL )
	{
		char *driverPath = qm_os_string_alloc( "local://%s", exePath );
		PlgScanForDrivers( driverPath );
		qm_os_memory_free( driverPath );
	}
	else
	{
		PlgScanForDrivers( "." );
	}

	SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );

	unsigned int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED;
	if ( !PlHasCommandLineArgument( "/nodpi" ) )
	{
		flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	}

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

	const char *projectName = com_project_get_name();
	char       *title       = qm_os_string_alloc( "%s (%s)", GRAFT_NAME, projectName );

	SDL_Window *window;
	if ( ( window = SDL_CreateWindow( title, 640, 480, flags ) ) == nullptr )
	{
		fprintf( stderr, "Failed to create SDL window: %s\n", SDL_GetError() );
	}

	qm_os_memory_free( title );

	if ( window == nullptr )
	{
		return false;
	}

	if ( ( glContext = SDL_GL_CreateContext( window ) ) == nullptr )
	{
		SDL_DestroyWindow( window );
		fprintf( stderr, "Failed to create GL context: %s\n", SDL_GetError() );
		return false;
	}

	SDL_GL_MakeCurrent( window, glContext );
	SDL_GL_SetSwapInterval( -1 );

	//TODO: we're just hardcoding this until this is all redone
	static constexpr char DRIVER[] = "opengl";
	if ( PlgSetDriver( DRIVER ) != PL_RESULT_SUCCESS )
	{
		return false;
	}

	return true;
}

int qm_os_main( const int argc, char **argv )
{
	if ( !SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO ) )
	{
		fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
		return EXIT_FAILURE;
	}

	aux_initialize( argc, argv );

	const char *appDir = com_get_app_data_directory();
	qm_fs_mount_local_location( appDir );
	const char *localDir = com_get_local_data_directory();
	qm_fs_mount_local_location( localDir );

	const char *projectName;
	if ( ( projectName = PlGetCommandLineArgumentValue( "/project" ) ) == nullptr )
	{
		projectName = "base";
	}

	if ( com_project_mount( projectName ) == nullptr )
	{
		fprintf( stderr, "Failed to mount project (%s)!\n", projectName );
		return EXIT_FAILURE;
	}

	// setup the main window
	if ( !setup_display() )
	{
		fprintf( stderr, "Failed to setup display!\n" );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

QM_OS_SYSTEM_IMPLEMENT_MAIN()
