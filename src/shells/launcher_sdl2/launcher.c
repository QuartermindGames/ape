// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <SDL3/SDL.h>

#ifdef _WIN32
#	include <crtdbg.h>
#elif __linux__
#	include <sys/prctl.h>
#	include <signal.h>
#endif

#include <acm/acm.h>

#include "qmos/public/qm_os_string.h"

#include "aux/public/aux.h"
#include "aux/public/aux_project.h"
#include "aux/public/aux_log.h"

#include "launcher.h"

#include "core/public/yin/core.h"
#include "core/public/core_console.h"

#include "shells/sdl3/shell_sdl3.c"

static AcmBranch *shellConfig;

/****************************************
 * WINDOW MANAGEMENT
 ****************************************/

static SDL_Window   *sdlWindow;
static SDL_GLContext sdlGLContext;

static ApeViewport *windowViewport;

static int drawW, drawH;

float shell_get_display_scale()
{
	if ( sdlWindow == nullptr )
	{
		return 1.0f;
	}

	return SDL_GetWindowDisplayScale( sdlWindow );
}

static SDL_Window *create_window( const char *title, int width, int height, bool fullscreen, uint8_t mode )
{
#if !defined( NDEBUG )
	int flags = SDL_WINDOW_RESIZABLE;
#else
	int flags = 0;
#endif
	if ( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	if ( !PlHasCommandLineArgument( "/nodpi" ) )
	{
		flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	}

	switch ( mode )
	{
		default:
			PrintWarn( "Unknown graphics mode (%d)!\n", mode );
			break;
		case SS_SHELL_GRAPHICS_MODE_OPENGL:
			flags |= SDL_WINDOW_OPENGL;
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
			SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
			SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );
			break;
		case SS_SHELL_GRAPHICS_MODE_VULKAN:
			flags |= SDL_WINDOW_VULKAN;
			break;
	}

	sdlWindow = SDL_CreateWindow( title, width, height, flags );
	if ( sdlWindow == NULL )
	{
		PrintWarn( "Failed to create SDL window: %s\n", SDL_GetError() );
		return nullptr;
	}

#if 0
#	if !NDEBUG// for debug builds, throw it onto a second display if available
	if ( SDL_GetNumVideoDisplays() > 1 )
	{
		SDL_SetWindowPosition( sdlWindow, SDL_WINDOWPOS_CENTERED_DISPLAY( 1 ), SDL_WINDOWPOS_CENTERED_DISPLAY( 1 ) );
		SDL_MaximizeWindow( sdlWindow );
	}
#	endif
#endif

	if ( mode == SS_SHELL_GRAPHICS_MODE_OPENGL )
	{
		sdlGLContext = SDL_GL_CreateContext( sdlWindow );
		if ( sdlGLContext == NULL )
		{
			SDL_DestroyWindow( sdlWindow );
			PrintWarn( "Failed to create OpenGL context: %s\n", SDL_GetError() );
			return nullptr;
		}

		SDL_GL_MakeCurrent( sdlWindow, sdlGLContext );
		SDL_GL_SetSwapInterval( 0 );
	}

	if ( !SDL_GetWindowSizeInPixels( sdlWindow, &drawW, &drawH ) )
	{
		PrintWarn( "Failed to get window pixel size: %s\n", SDL_GetError() );
		drawW = 640;
		drawH = 480;
	}

	return sdlWindow;
}

#if 0
static void DestroyWindow( void )
{
	if ( sdlGLContext != NULL )
		SDL_GL_DeleteContext( sdlGLContext );

	if ( sdlWindow != NULL )
		SDL_DestroyWindow( sdlWindow );
}
#endif

bool ss_shell_set_window_size( int *width, int *height )
{
	if ( sdlWindow == NULL )
	{
		*width  = 0;
		*height = 0;
		return false;
	}

	SDL_SetWindowSize( sdlWindow, *width, *height );

	int nW, nH;
	SDL_GetWindowSize( sdlWindow, &nW, &nH );

	if ( *width == nW && *height == nH )
		return true;

	*width  = nW;
	*height = nH;
	return false;
}

void shell_get_window_size( int *width, int *height )
{
	SDL_GetWindowSize( sdlWindow, width, height );
}

void ss_shell_set_window_icon( const QmImage *image )
{
	SDL_Surface *surface = SDL_CreateSurfaceFrom( ( int ) image->width, ( int ) image->height, SDL_PIXELFORMAT_RGBA8888, image->data[ 0 ], ( int ) image->width * 4 );
	if ( surface == NULL )
	{
		PrintWarn( "Failed to create requested SDL surface: %s\n", SDL_GetError() );
		return;
	}

	SDL_SetWindowIcon( sdlWindow, surface );
	SDL_DestroySurface( surface );
}

ApeViewport *ss_shell_viewport_get_active( void )
{
	return windowViewport;
}

/****************************************
 * INPUT MANAGEMENT
 ****************************************/

void shell_set_mouse_position( int x, int y )
{
	SDL_WarpMouseInWindow( sdlWindow, x, y );
}

void ss_shell_grab_mouse( bool grab )
{
	SDL_SetWindowMouseGrab( sdlWindow, grab );
	SDL_SetWindowRelativeMouseMode( sdlWindow, grab );

	if ( grab )
	{
		SDL_ShowCursor();
	}
	else
	{
		SDL_HideCursor();
	}
}

/****************************************
 * INITIALIZATION
 ****************************************/

void ss_shell_shutdown( void )
{
	com_write_config( shellConfig, "shell" );

	if ( sdlGLContext != nullptr )
	{
		SDL_GL_DestroyContext( sdlGLContext );
		sdlGLContext = nullptr;
	}

	if ( sdlWindow != nullptr )
	{
		SDL_DestroyWindow( sdlWindow );
		sdlWindow = nullptr;
	}

	shell_shutdown();
}

int launcherLog;

static bool initialize_display( void )
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
		PrintWarn( "Failed to get executable location: %s\n", PlGetError() );
		PlgScanForDrivers( "." );
	}

	unsigned int driverMode;
	const char  *driverName = acm_get_string( shellConfig, "shell.driver", "opengl" );
	if ( strcmp( driverName, "opengl" ) == 0 )
	{
		driverMode = SS_SHELL_GRAPHICS_MODE_OPENGL;
	}
	else if ( strcmp( driverName, "vulkan" ) == 0 )
	{
		driverMode = SS_SHELL_GRAPHICS_MODE_VULKAN;
	}
	else if ( strcmp( driverName, "software" ) == 0 )
	{
		driverMode = SS_SHELL_GRAPHICS_MODE_SOFTWARE;
	}
	else
	{
		driverMode = SS_SHELL_GRAPHICS_MODE_OTHER;
	}

	SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );

#if !NDEBUG
	bool fullscreen = true;
#else
	bool fullscreen = true;
#endif
	int width  = 1920;
	int height = 1080;
	if ( shellConfig != NULL )
	{
		fullscreen = acm_get_bool( shellConfig, "fullscreen", fullscreen );
		width      = ( int ) acm_get_int( shellConfig, "width", width );
		height     = ( int ) acm_get_int( shellConfig, "height", height );
	}

	if ( PlHasCommandLineArgument( "/window" ) )
	{
		fullscreen = false;
	}
	else if ( PlHasCommandLineArgument( "/fullscreen" ) )
	{
		fullscreen = true;
	}

	const char *arg;
	if ( ( arg = PlGetCommandLineArgumentValue( "/width" ) ) != nullptr )
	{
		width = ( int ) strtol( arg, nullptr, 10 );
	}
	if ( ( arg = PlGetCommandLineArgumentValue( "/height" ) ) != nullptr )
	{
		height = ( int ) strtol( arg, nullptr, 10 );
	}

	if ( ( sdlWindow = create_window( com_project_get_name(), width, height, fullscreen, driverMode ) ) == NULL )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to create window!\n" );
		return EXIT_FAILURE;
	}

	if ( PlgSetDriver( driverName ) != PL_RESULT_SUCCESS )
	{
		if ( strcmp( driverName, "software" ) != 0 )
		{
			PrintWarn( "Driver init failed for \"%s\": %s\n", driverName, PlGetError() );
			Print( "Attempting to use software fallback...\n" );
			if ( PlgSetDriver( "software" ) != PL_RESULT_SUCCESS )
			{
				PrintWarn( "Failed to set fallback driver: %s\n", PlGetError() );
				return false;
			}
		}
		else
		{
			PrintWarn( "Driver init failed for \"%s\" and fallback failed: %s\n", driverName, PlGetError() );
			return false;
		}
	}

	if ( !SDL_StartTextInput( sdlWindow ) )
	{
		PrintError( "Failed to start text input: %s\n", SDL_GetError() );
	}

	return true;
}

int qm_os_main( const int argc, char **argv )
{
#if defined( _WIN32 ) && !defined( NDEBUG )
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	setvbuf( stdout, NULL, _IONBF, 0 );
#elif defined( __linux__ )
	prctl( PR_SET_DUMPABLE, 1 );
#endif

	if ( !shell_initialize( argc, argv ) )
	{
		return EXIT_FAILURE;
	}

	Print( "Log output initialized!\n" );

	if ( !initialize_display() )
	{
		PrintError( "Failed to initialize display!\nCheck debug logs.\n" );
	}

	if ( !ape_initialize( argc, argv, nullptr ) )
	{
		PrintError( "Failed to initialize engine!\nCheck debug logs.\n" );
	}

	launcherLog = aux_log_register_source( "launcher", PL_COLOUR_CORAL, true );

	int w, h;
	shell_get_window_size( &w, &h );
	if ( ( windowViewport = ape_viewport_create( 0, 0, w, h, sdlWindow, true ) ) == NULL )
	{
		PrintError( "Failed to create virtual window viewport!\n" );
	}

	// setup our timers, in this case we're just setting up our tick

	if ( !shell_setup_tick_timer() )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to setup ticket timer!" );
		return EXIT_FAILURE;
	}

	// this variable denotes whether we should only draw a frame on tick, or always draw
	PL_GET_CVAR( "renderTimeLock", renderTimeLockVar );

	static bool shouldDraw = true;
	while ( ape_is_running() )
	{
		COM_PROFILE_START( "frametime" );

		static bool updateProfiler;

		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			switch ( event.type )
			{
				default:
					break;

				case SDL_EVENT_USER:
					ape_tick_frame();
					shouldDraw     = true;
					updateProfiler = true;
					break;

				case SDL_EVENT_TEXT_INPUT:
					ape_input_handle_text_event( event.text.text );
					break;

				case SDL_EVENT_MOUSE_WHEEL:
				{
					float x = event.wheel.x > 0 ? 1.0f : event.wheel.x < 0 ? -1.0f
					                                                       : 0.0f;
					float y = event.wheel.y > 0 ? 1.0f : event.wheel.y < 0 ? -1.0f
					                                                       : 0.0f;
					ape_input_handle_mouse_wheel_event( x, y );
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_MOUSE_BUTTON_UP:
					ape_input_handle_mouse_button_event( event.button.button, event.button.type == SDL_EVENT_MOUSE_BUTTON_DOWN );
					break;
				case SDL_EVENT_MOUSE_MOTION:
					ape_input_handle_mouse_motion_event( event.motion.x, event.motion.y );
					break;

				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP:
				{
					int key = shell_translate_key_input( event.key.key );
					if ( key == KEY_INVALID )
					{
						//PrintWarn( "Unhandled key, %d\n", key );
						break;
					}

					ape_input_handle_keyboard_event( key, event.type == SDL_EVENT_KEY_DOWN );
					break;
				}

				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				{
					if ( sdlWindow == NULL || event.window.windowID != SDL_GetWindowID( sdlWindow ) )
					{
						break;
					}

					SDL_GetWindowSizeInPixels( sdlWindow, &drawW, &drawH );
					ape_viewport_set_size( windowViewport, drawW, drawH );
					break;
				}
			}
		}

		if ( !renderTimeLockVar->b_value || ( renderTimeLockVar->b_value && shouldDraw ) )
		{
			ape_render_frame( windowViewport );

			SDL_GL_SwapWindow( sdlWindow );
			shouldDraw = false;

			updateProfiler = true;
		}

		COM_PROFILE_END( "frametime" );

		if ( updateProfiler )
		{
			//TODO: ditch this once console interface is in aux?
			static bool           checkFail;
			static ApeConsoleVar *profilerFrequency;
			static unsigned int   freq = 32;
			if ( profilerFrequency == nullptr && !checkFail )
			{
				profilerFrequency = PlGetConsoleVariable( "debug/profilerFrequency" );
				if ( profilerFrequency == nullptr )
				{
					// if this fails, never check again
					checkFail = true;
				}
			}

			if ( profilerFrequency != nullptr )
			{
				const char *c = ape_console_var_get( "debug/profilerFrequency" );
				freq          = strtol( c, nullptr, 10 );
			}

			com_profiler_update_samples( freq );
			updateProfiler = false;
		}
	}

	SDL_StopTextInput( sdlWindow );

	ape_shutdown();

	aux_shutdown();

	return EXIT_SUCCESS;
}

QM_OS_SYSTEM_IMPLEMENT_MAIN()
