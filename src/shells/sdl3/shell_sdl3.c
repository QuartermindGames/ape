// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: SDL3 helper methods.
// Author:  Mark E. Sowden

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_timer.h>

#include <stdio.h>

#include "qmos/public/qm_os_memory.h"

#include "plcore/pl_filesystem.h"
#include "plcore/pl_console.h"

#include "acm/acm.h"

#include "aux/public/aux.h"
#include "aux/public/aux_project.h"

#include "yin/core_input.h"
#include "yin/core_shell.h"

static AcmBranch *shellConfig;

static PLConsoleVariable *tickFrequencyVar;

static SDL_TimerID  tickTimer;
static unsigned int tick_timer_callback( void *userData, SDL_TimerID timer, uint32_t interval )
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

	SDL_ShowSimpleMessageBox( flags, title, buf, nullptr );

	qm_os_memory_free( buf );
}

bool shell_initialize( unsigned int argc, char **argv )
{
	if ( !SDL_Init( SDL_INIT_EVENTS | SDL_INIT_VIDEO ) )
	{
		fprintf( stderr, "Failed to initialize SDL: %s\n", SDL_GetError() );
		return false;
	}

	aux_initialize( argc, argv );

	const char *appDir = com_get_app_data_directory();
	qm_fs_mount_local_location( appDir );
	const char *localDir = com_get_local_data_directory();
	qm_fs_mount_local_location( localDir );

	shellConfig = com_get_config( "shell" );

	const char *projectName;
	if ( ( projectName = PlGetCommandLineArgumentValue( "/project" ) ) == NULL )
	{
		projectName = acm_get_string( shellConfig, "defaultProject", "base" );
	}

	if ( com_project_mount( projectName ) == nullptr )
	{
		fprintf( stderr, "Failed to mount project (%s)!\n", projectName );
		return false;
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
			fprintf( stderr, "Failed to execute cook command!\n" );
			return false;
		}
	}
#endif

	SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );

	return true;
}

bool shell_setup_tick_timer()
{
	tickFrequencyVar = PlGetConsoleVariable( "tickFrequency" );
	if ( tickFrequencyVar == nullptr )
	{
		fprintf( stderr, "No tick frequency variable found: %s\n", PlGetError() );
		return false;
	}

	if ( ( tickTimer = SDL_AddTimer( tickFrequencyVar->i_value, tick_timer_callback, NULL ) ) == 0 )
	{
		fprintf( stderr, "Failed to setup timer: %s\n", SDL_GetError() );
		return false;
	}

	return true;
}

void shell_shutdown()
{
	if ( tickTimer != 0 )
	{
		SDL_RemoveTimer( tickTimer );
	}
}

int shell_translate_key_input( const int key )
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

/////////////////////////////////////////////////////////////////////////////////////
// Window
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ShellWindow
{
	ApeViewport *viewport;
	SDL_Window  *handle;
} ShellWindow;

static void shell_window_destroy( void *ptr )
{
	ShellWindow *window = ptr;
	if ( window->handle != nullptr )
	{
		SDL_DestroyWindow( window->handle );
		window->handle = nullptr;
	}
}

ShellWindow *shell_window_create( const char *title, unsigned int w, unsigned int h, bool fs )
{
	unsigned int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	if ( fs )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	if ( !PlHasCommandLineArgument( "/nodpi" ) )
	{
		flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
	}

	SDL_Window *handle = SDL_CreateWindow( title, w, h, flags );
	if ( handle == nullptr )
	{
		fprintf( stderr, "Failed to create SDL window: %s\n", SDL_GetError() );
		return nullptr;
	}

	ShellWindow *window = QM_OS_MEMORY_NEW_D( ShellWindow, shell_window_destroy );
	if ( window == nullptr )
	{
		SDL_DestroyWindow( handle );

		fprintf( stderr, "Failed to alloc shell window!\n" );
		return nullptr;
	}

	if ( !SDL_StartTextInput( handle ) )
	{
		SDL_DestroyWindow( handle );

		fprintf( stderr, "Failed to start text input: %s\n", SDL_GetError() );
		return nullptr;
	}

	SDL_ShowWindow( handle );

	window->handle = handle;

	return window;
}

void shell_window_maximize( const ShellWindow *self )
{
	SDL_MaximizeWindow( self->handle );
}

void shell_window_get_size( const ShellWindow *self, unsigned int *dstW, unsigned int *dstH )
{
	SDL_GetWindowSize( self->handle, ( int * ) dstW, ( int * ) dstH );
}
