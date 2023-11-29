// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <SDL2/SDL.h>

#ifdef _WIN32
#	include <crtdbg.h>
#elif __linux__
#	include <sys/prctl.h>
#endif

#include <yin/core.h>
#include <yin/core_renderer.h>
#include <yin/node.h>

#include "common.h"
#include "common_project.h"
#include "launcher.h"

static NdBranch *shellConfig;

void ss_shell_push_message( int level, const char *msg, const PLColour *colour )
{
}

/****************************************
 * WINDOW MANAGEMENT
 ****************************************/

static SDL_Window *sdlWindow = NULL;
static SDL_GLContext sdlGLContext = NULL;

static SSArlViewport *windowViewport = NULL;

static int drawW, drawH;

void ss_shell_display_message( SS_Shell_MessageBoxType messageType, const char *message, ... )
{
	const char *title;
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

	int l = pl_vscprintf( message, args );
	char *buf = PlMAllocA( l + 1 );

	vsnprintf( buf, l, message, args );

	va_end( args );

	PlLogWFunction( launcherLog, "%s", buf );

	SDL_ShowSimpleMessageBox( flags, title, buf, NULL );

	PL_DELETE( buf );
}

static bool IsWindowActive( void )
{
	assert( sdlWindow != NULL );
	uint32_t flags = SDL_GetWindowFlags( sdlWindow );
	return ( !( flags & SDL_WINDOW_HIDDEN ) && ( flags & SDL_WINDOW_INPUT_FOCUS ) );
}

SDL_Window *create_window( const char *title, int width, int height, bool fullscreen, uint8_t mode )
{
	int flags = 0;
#if !NDEBUG
	flags |= SDL_WINDOW_RESIZABLE;
#endif
	if ( fullscreen )
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

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

	sdlWindow = SDL_CreateWindow(
	        title,
	        SDL_WINDOWPOS_CENTERED,
	        SDL_WINDOWPOS_CENTERED,
	        width, height,
	        flags );
	if ( sdlWindow == NULL )
	{
		PrintWarn( "Failed to create SDL window: %s\n", SDL_GetError() );
		return false;
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
			return false;
		}

		SDL_GL_MakeCurrent( sdlWindow, sdlGLContext );
		SDL_GL_GetDrawableSize( sdlWindow, &drawW, &drawH );
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
		*width = 0;
		*height = 0;
		return false;
	}

	SDL_SetWindowSize( sdlWindow, *width, *height );

	int nW, nH;
	SDL_GetWindowSize( sdlWindow, &nW, &nH );

	if ( *width == nW && *height == nH )
		return true;

	*width = nW;
	*height = nH;
	return false;
}

void ss_shell_get_window_size( int *width, int *height )
{
	SDL_GetWindowSize( sdlWindow, width, height );
}

void ss_shell_set_window_icon( const PLImage *image )
{
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
	        image->data[ 0 ],
	        ( signed ) image->width,
	        ( signed ) image->height,
	        32,
	        ( signed ) image->width * 4,
	        0x000000ff,
	        0x0000ff00,
	        0x00ff0000,
	        0xff000000 );
	if ( surface == NULL )
	{
		PrintWarn( "Failed to create requested SDL surface: %s\n", SDL_GetError() );
		return;
	}

	SDL_SetWindowIcon( sdlWindow, surface );
	SDL_FreeSurface( surface );
}

SSArlViewport *ss_shell_viewport_get_active( void )
{
	return windowViewport;
}

void ss_shell_swap_window( SSArlViewport * )
{
	// Just ignore the viewport for now...
	SDL_GL_SwapWindow( sdlWindow );
}

/****************************************
 * INPUT MANAGEMENT
 ****************************************/

static ApeInputState buttonStates[ APE_MAX_BUTTON_INPUTS ];
ApeInputState ss_shell_get_button_state( ApeInputButton inputButton )
{
	if ( inputButton >= APE_MAX_BUTTON_INPUTS )
		return APE_INPUT_STATE_NONE;

	return buttonStates[ inputButton ];
}

static ApeInputState keyStates[ APE_MAX_KEY_INPUTS ];
ApeInputState ss_shell_get_key_state( int key )
{
	if ( key >= APE_MAX_KEY_INPUTS )
		return APE_INPUT_STATE_NONE;

	return keyStates[ key ];
}

void ss_shell_get_mouse_position( int *x, int *y )
{
	SDL_GetMouseState( x, y );
}

void ss_shell_set_mouse_position( int x, int y )
{
	SDL_WarpMouseInWindow( sdlWindow, x, y );
}

static bool grabState = false;

void ss_shell_grab_mouse( bool grab )
{
	SDL_SetWindowGrab( sdlWindow, grab );
	SDL_SetRelativeMouseMode( grab );

	SDL_ShowCursor( !grab );
}

static int Sys_TranslateSDLKeyInput( int key )
{
	if ( key < 128 )
		return key;

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
			return KEY_UP;
		case SDLK_DOWN:
			return KEY_DOWN;
		case SDLK_LEFT:
			return KEY_LEFT;
		case SDLK_RIGHT:
			return KEY_RIGHT;
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

			/* temp temp temp */
		case SDLK_ESCAPE:
			ss_acl_shutdown();
			break;
	}

	return KEY_INVALID;
}

/****************************************
 * TIMER MANAGEMENT
 ****************************************/

static SDL_TimerID sdlTimer = 0;
static unsigned int timer_callback( unsigned int interval, void *param )
{
	SDL_UserEvent userEvent;
	userEvent.type = SDL_USEREVENT;
	userEvent.code = 0;

	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user = userEvent;

	SDL_PushEvent( &event );

	return interval;
}

/****************************************
 * INITIALIZATION
 ****************************************/

void ss_shell_shutdown( void )
{
	comWriteConfig( shellConfig, "shell" );

	if ( sdlTimer != 0 )
		SDL_RemoveTimer( sdlTimer );

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
		size_t size = strlen( exePath ) + PL_SYSTEM_MAX_PATH + 1;
		char *driverPath = PL_NEW_( char, size );
		snprintf( driverPath, size, "local://%s", exePath );
		PlgScanForDrivers( driverPath );
		PL_DELETE( driverPath );
	}
	else
	{
		PrintWarn( "Failed to get executable location: %s\n", PlGetError() );
		PlgScanForDrivers( "." );
	}

	unsigned int driverMode;
	const char *driverName = ndGetStringByName( shellConfig, "shell.driver", "opengl" );
	if ( strcmp( driverName, "opengl" ) == 0 )
		driverMode = SS_SHELL_GRAPHICS_MODE_OPENGL;
	else if ( strcmp( driverName, "vulkan" ) == 0 )
		driverMode = SS_SHELL_GRAPHICS_MODE_VULKAN;
	else if ( strcmp( driverName, "software" ) == 0 )
		driverMode = SS_SHELL_GRAPHICS_MODE_SOFTWARE;
	else
		driverMode = SS_SHELL_GRAPHICS_MODE_OTHER;

	const char *windowTitle = com_project_get_name();
	if ( windowTitle == NULL )
		windowTitle = "APE Game Shell";

	SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );

	if ( ( sdlWindow = create_window( windowTitle, 1024, 768, true, driverMode ) ) == NULL )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to create window!\n" );
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

	if ( PlHasCommandLineArgument( "-log" ) )
	{
		const char *path = PlGetCommandLineArgumentValue( "-log" );
		if ( path == NULL )
			path = "log.txt";

		PlSetupLogOutput( path );
	}

	launcherLog = PlAddLogLevel( "launcher", PL_COLOUR_WHITE, true );
	Print( "Log output initialized!\n" );

	if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 )
		PrintError( "Failed to initialize SDL2!\nSDL: %s\n", SDL_GetError() );

	com_initialize();

	shellConfig = com_get_config( "shell" );

	const char *projectName;
	if ( ( projectName = PlGetCommandLineArgumentValue( "/project" ) ) == NULL )
		projectName = "base";

	com_project_mount( projectName );

	if ( !initialize_display() )
		PrintError( "Failed to initialize display!\nCheck debug logs.\n" );

	if ( !ss_acl_initialize( argc, argv, NULL ) )
		PrintError( "Failed to initialize engine!\nCheck debug logs.\n" );

	int w, h;
	ss_shell_get_window_size( &w, &h );
	windowViewport = ss_arl_viewport_create( 0, 0, w, h, sdlWindow );
	if ( windowViewport == NULL )
		PrintError( "Failed to create virtual window viewport!\n" );

	// setup our timers, in this case we're just setting up our tick
	sdlTimer = SDL_AddTimer( SS_SHELL_TICK_RATE, timer_callback, NULL );

	SDL_StartTextInput();

	while ( ss_acl_is_engine_running() )
	{
		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			switch ( event.type )
			{
				case SDL_USEREVENT:
					ss_acl_tick_frame();
					break;

				case SDL_TEXTINPUT:
					ss_acl_input_handle_text_event( event.text.text );
					break;

				case SDL_MOUSEWHEEL:
				{
					float x = ( event.wheel.x > 0 ) ? 1.0f : ( event.wheel.x < 0 ) ? -1.0f
					                                                               : 0.0f;
					float y = ( event.wheel.y > 0 ) ? 1.0f : ( event.wheel.y < 0 ) ? -1.0f
					                                                               : 0.0f;
					ss_acl_input_handle_mouse_wheel_event( x, y );
					break;
				}
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					ss_acl_input_handle_mouse_button_event( event.button.button, ( event.button.type == SDL_MOUSEBUTTONDOWN ) );
					break;
				case SDL_MOUSEMOTION:
					ss_acl_input_handle_mouse_motion_event( event.motion.x, event.motion.y );
					break;

				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					int key = Sys_TranslateSDLKeyInput( event.key.keysym.sym );
					if ( key == KEY_INVALID )
					{
						//PrintWarn( "Unhandled key, %d\n", key );
						break;
					}

					keyStates[ key ] = ( event.type == SDL_KEYDOWN ) ? APE_INPUT_STATE_DOWN : APE_INPUT_STATE_NONE;

					ss_acl_input_handle_keyboard_event( key, keyStates[ key ] );
					break;
				}

				case SDL_WINDOWEVENT:
				{
					if ( sdlWindow == NULL || event.window.windowID != SDL_GetWindowID( sdlWindow ) )
						break;

					switch ( event.window.event )
					{
						case SDL_WINDOWEVENT_RESIZED:
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							//SDL_GL_GetDrawableSize( sdlWindow, &drawW, &drawW );
							// originally used the above but it kept returning bogus coords...
							SDL_GetWindowSize( sdlWindow, &drawW, &drawH );
							ss_arl_viewport_set_size( windowViewport, drawW, drawH );
							break;
					}
					break;
				}
			}
		}

		ss_acl_render_frame( windowViewport );

		static unsigned int refreshTime = 0;
		if ( refreshTime > ss_acl_get_num_ticks() )
			continue;

		com_update_profiler_samples();

		PL_GET_CVAR( "debug/profilerFrequency", profilerFrequency );
		refreshTime += ( profilerFrequency != NULL ) ? profilerFrequency->i_value : 16;
	}

	SDL_StopTextInput();

	ss_acl_shutdown();

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
