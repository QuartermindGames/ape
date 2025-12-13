// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <SDL3/SDL.h>

#ifdef _WIN32
#	include <crtdbg.h>
#elif __linux__
#	include <sys/prctl.h>
#	include <signal.h>
#endif

#include <acm/acm.h>

#include <yin/core.h>

#include "common.h"
#include "common_project.h"
#include "launcher.h"

static AcmBranch *shellConfig;

static PLConsoleVariable *tickFrequencyVar;

/****************************************
 * WINDOW MANAGEMENT
 ****************************************/

static SDL_Window   *sdlWindow    = nullptr;
static SDL_GLContext sdlGLContext = nullptr;

static ApeViewport *windowViewport = nullptr;

static int drawW, drawH;

void shell_display_message( SS_Shell_MessageBoxType messageType, const char *message, ... )
{
	const char         *title;
	SDL_MessageBoxFlags flags;
	switch ( messageType )
	{
		case SS_SHELL_MESSAGE_BOX_TYPE_ERROR:
			title = "Error";
			flags = SDL_MESSAGEBOX_ERROR;
			break;
		case SS_SHELL_MESSAGE_BOX_TYPE_WARNING:
			title = "Warning";
			flags = SDL_MESSAGEBOX_WARNING;
			break;
		default:
		case SS_SHELL_MESSAGE_BOX_TYPE_INFO:
			title = "Info";
			flags = SDL_MESSAGEBOX_INFORMATION;
			break;
	}

	va_list args;
	va_start( args, message );

	int   l   = pl_vscprintf( message, args );
	char *buf = QM_OS_MEMORY_MALLOC_( l + 1 );

	vsnprintf( buf, l, message, args );

	va_end( args );

	PlLogWFunction( launcherLog, "%s", buf );

	SDL_ShowSimpleMessageBox( flags, title, buf, nullptr );

	qm_os_memory_free( buf );
}

float shell_get_display_scale()
{
	if ( sdlWindow == nullptr )
	{
		return 1.0f;
	}

	return SDL_GetWindowDisplayScale( sdlWindow );
}

static bool IsWindowActive( void )
{
	assert( sdlWindow != NULL );
	uint32_t flags = SDL_GetWindowFlags( sdlWindow );
	return ( !( flags & SDL_WINDOW_HIDDEN ) && ( flags & SDL_WINDOW_INPUT_FOCUS ) );
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

void ss_shell_set_window_icon( const PLImage *image )
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

static ApeInputState buttonStates[ APE_MAX_BUTTON_INPUTS ];
ApeInputState        ss_shell_get_button_state( ApeInputButton inputButton )
{
	if ( inputButton >= APE_MAX_BUTTON_INPUTS )
		return APE_INPUT_STATE_NONE;

	return buttonStates[ inputButton ];
}

static ApeInputState keyStates[ APE_MAX_KEY_INPUTS ];
ApeInputState        ss_shell_get_key_state( int key )
{
	if ( key >= APE_MAX_KEY_INPUTS )
		return APE_INPUT_STATE_NONE;

	return keyStates[ key ];
}

void shell_set_mouse_position( int x, int y )
{
	SDL_WarpMouseInWindow( sdlWindow, x, y );
}

static bool grabState = false;

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

static int Sys_TranslateSDLKeyInput( int key )
{
	switch ( key )
	{
		default:
			break;
		case SDLK_CAPSLOCK:
			return KEY_CAPSLOCK;
		case SDLK_F1:
			return KEY_F1;
		case SDLK_F2:
			return KEY_F2;
		case SDLK_F3:
			return KEY_F3;
		case SDLK_F4:
			return KEY_F4;
		case SDLK_F5:
			return KEY_F5;
		case SDLK_F6:
			return KEY_F6;
		case SDLK_F7:
			return KEY_F7;
		case SDLK_F8:
			return KEY_F8;
		case SDLK_F9:
			return KEY_F9;
		case SDLK_F10:
			return KEY_F10;
		case SDLK_F11:
			return KEY_F11;
		case SDLK_F12:
			return KEY_F12;
		case SDLK_PRINTSCREEN:
			return KEY_PRINTSCREEN;
		case SDLK_SCROLLLOCK:
			return KEY_SCROLLLOCK;
		case SDLK_PAUSE:
			return KEY_PAUSE;
		case SDLK_INSERT:
			return KEY_INSERT;
		case SDLK_HOME:
			return KEY_HOME;
		case SDLK_PAGEUP:
			return KEY_PAGEUP;
		case SDLK_PAGEDOWN:
			return KEY_PAGEDOWN;
		case SDLK_DELETE:
			return KEY_DELETE;
		case SDLK_END:
			return KEY_END;
		case SDLK_KP_TAB:
		case SDLK_TAB:
			return KEY_TAB;
		case SDLK_KP_ENTER:
			return KEY_ENTER;
		case SDLK_UP:
			return APE_INPUT_KEY_UP;
		case SDLK_DOWN:
			return APE_INPUT_KEY_DOWN;
		case SDLK_LEFT:
			return APE_INPUT_KEY_LEFT;
		case SDLK_RIGHT:
			return APE_INPUT_KEY_RIGHT;
		case SDLK_LCTRL:
			return KEY_LEFT_CTRL;
		case SDLK_RCTRL:
			return KEY_RIGHT_CTRL;
		case SDLK_LSHIFT:
			return KEY_LEFT_SHIFT;
		case SDLK_RSHIFT:
			return KEY_RIGHT_SHIFT;
		case SDLK_LALT:
			return KEY_LEFT_ALT;
		case SDLK_RALT:
			return KEY_RIGHT_ALT;
		case SDLK_ESCAPE:
			return APE_INPUT_KEY_ESCAPE;
	}

	if ( key < 128 )
	{
		return key;
	}

	return KEY_INVALID;
}

/****************************************
 * TIMER MANAGEMENT
 ****************************************/

static SDL_TimerID  sdlTimer = 0;
static unsigned int timer_callback( void *userData, SDL_TimerID timerId, uint32_t interval )
{
	SDL_UserEvent userEvent = {};
	userEvent.type          = SDL_EVENT_USER;
	userEvent.code          = 0;

	SDL_Event event;
	event.type = SDL_EVENT_USER;
	event.user = userEvent;

	SDL_PushEvent( &event );

	assert( tickFrequencyVar->i_value > 0 );
	return tickFrequencyVar->i_value;
}

/****************************************
 * INITIALIZATION
 ****************************************/

void ss_shell_shutdown( void )
{
	com_write_config( shellConfig, "shell" );

	if ( sdlTimer != 0 )
	{
		SDL_RemoveTimer( sdlTimer );
	}

	exit( EXIT_SUCCESS );
}

int launcherLog;

static bool initialize_display( void )
{
	PlgInitializeGraphics();

	// attempt to fetch the driver directly from the executable location if possible
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) != NULL )
	{
		size_t size       = strlen( exePath ) + PL_SYSTEM_MAX_PATH + 1;
		char  *driverPath = QM_OS_MEMORY_NEW_( char, size );
		snprintf( driverPath, size, "local://%s", exePath );
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

	const char *windowTitle = com_project_get_name();
	if ( windowTitle == NULL )
	{
		windowTitle = "APE Game Shell";
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

	if ( ( sdlWindow = create_window( windowTitle, width, height, fullscreen, driverMode ) ) == NULL )
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

	return true;
}

int launcher_initialize( int argc, char **argv )
{
#if defined( _WIN32 ) && !defined( NDEBUG )
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	setvbuf( stdout, NULL, _IONBF, 0 );
#elif defined( __linux__ )
	prctl( PR_SET_DUMPABLE, 1 );
#endif

	/* initialize the platform library */
	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
	{
		printf( "Failed to initialize Hei: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	if ( PlInitializeSubSystems( PL_SUBSYSTEM_IO ) != PL_RESULT_SUCCESS )
	{
		printf( "Failed to initialize IO subsystem: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	launcherLog = PlAddLogLevel( "launcher", PL_COLOUR_WHITE, true );
	Print( "Log output initialized!\n" );

	if ( !SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO ) )
	{
		PrintError( "Failed to initialize SDL: %s\n", SDL_GetError() );
	}

	com_initialize();

	PlMountLocalLocation( com_get_app_data_directory() );
	PlMountLocalLocation( com_get_local_data_directory() );

	shellConfig = com_get_config( "shell" );

	const char *projectName;
	if ( ( projectName = PlGetCommandLineArgumentValue( "/project" ) ) == NULL )
	{
		projectName = acm_get_string( shellConfig, "defaultProject", "base" );
	}

	if ( com_project_mount( projectName ) == nullptr )
	{
		PrintError( "Failed to mount project (%s)!\n", projectName );
	}

#if !defined( _WIN32 )
	// allow us to cook everything before launching, if desired
	if ( PlHasCommandLineArgument( "/cook" ) )
	{
		PLPath exePath;
		PlGetExecutableDirectory( exePath, sizeof( exePath ) );

		char tmp[ sizeof( exePath ) + 64 ];
		snprintf( tmp, sizeof( tmp ), "%s/cook %s", exePath, projectName );
		if ( system( tmp ) == -1 )
		{
			PrintWarn( "Failed to execute cook command!\n" );
		}
	}
#endif

	if ( !initialize_display() )
	{
		PrintError( "Failed to initialize display!\nCheck debug logs.\n" );
	}

	if ( !ape_initialize( argc, argv, nullptr ) )
	{
		PrintError( "Failed to initialize engine!\nCheck debug logs.\n" );
	}

	int w, h;
	shell_get_window_size( &w, &h );
	windowViewport = ape_viewport_create( 0, 0, w, h, sdlWindow, true );
	if ( windowViewport == NULL )
	{
		PrintError( "Failed to create virtual window viewport!\n" );
	}

	// setup our timers, in this case we're just setting up our tick

	tickFrequencyVar = PlGetConsoleVariable( "tickFrequency" );
	if ( tickFrequencyVar == nullptr )
	{
		PrintError( "No tick frequency variable found: %s\n", PlGetError() );
	}

	if ( ( sdlTimer = SDL_AddTimer( tickFrequencyVar->i_value, timer_callback, NULL ) ) == 0 )
	{
		PrintError( "Failed to setup timer: %s\n", SDL_GetError() );
	}

	if ( !SDL_StartTextInput( sdlWindow ) )
	{
		PrintError( "Failed to start text input: %s\n", SDL_GetError() );
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
					float x = ( event.wheel.x > 0 ) ? 1.0f : ( event.wheel.x < 0 ) ? -1.0f
					                                                               : 0.0f;
					float y = ( event.wheel.y > 0 ) ? 1.0f : ( event.wheel.y < 0 ) ? -1.0f
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
					int key = Sys_TranslateSDLKeyInput( event.key.key );
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

					//SDL_GL_GetDrawableSize( sdlWindow, &drawW, &drawW );
					// originally used the above but it kept returning bogus coords...
					SDL_GetWindowSize( sdlWindow, &drawW, &drawH );
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
			static bool               checkFail;
			static PLConsoleVariable *profilerFrequency;
			static unsigned int       freq = 32;
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
				const char *c = PlGetConsoleVariableValue( "debug/profilerFrequency" );
				freq          = strtol( c, nullptr, 10 );
			}

			com_profiler_update_samples( freq );
			updateProfiler = false;
		}
	}

	SDL_StopTextInput( sdlWindow );

	ape_shutdown();

	return EXIT_SUCCESS;
}

#if defined( _WIN32 )

#	include <windows.h>

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow )
{
	return launcher_initialize( __argc, __argv );
}

#else

int main( int argc, char **argv )
{
	return launcher_initialize( argc, argv );
}

#endif
