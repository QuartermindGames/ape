/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * */

#include <SDL2/SDL.h>

#include "launcher.h"

EngineInterface g_engine;
static PLLibrary *dllEnginePtr;

#define PrintWarn( ... ) Sys_DisplayMessageBox( SYS_MESSAGE_WARNING, __VA_ARGS__ )
#define PrintError( ... )                                    \
	Sys_DisplayMessageBox( SYS_MESSAGE_ERROR, __VA_ARGS__ ); \
	exit( 0 )

/****************************************
 * WINDOW MANAGEMENT
 ****************************************/

void Sys_DisplayMessageBox( SysMessage messageType, const char *message, ... ) {
	const char *title;
	SDL_MessageBoxFlags flags;
	switch ( messageType ) {
		case SYS_MESSAGE_ERROR:
			title = "Error";
			flags = SDL_MESSAGEBOX_ERROR;
			break;
		case SYS_MESSAGE_WARNING:
			title = "Warning";
			flags = SDL_MESSAGEBOX_WARNING;
			break;
		case SYS_MESSAGE_INFO:
			title = "Info";
			flags = SDL_MESSAGEBOX_INFORMATION;
			break;
	}

	/* todo, make this dynamically sized */
	char msgBuf[ 4096 ];
	va_list args;
	va_start( args, message );
	vsnprintf( msgBuf, sizeof( msgBuf ), message, args );
	va_end( args );

	printf( "%s", msgBuf );

	SDL_ShowSimpleMessageBox( flags, title, msgBuf, NULL );
}

typedef struct SysWindow {
	SDL_Window *sdlWindowPtr;
	SDL_GLContext *sdlGLContext;
} SysWindow;

void Sys_GetWindowSize( SysWindow *windowPtr, int *width, int *height ) {
	SDL_GL_GetDrawableSize( windowPtr->sdlWindowPtr, width, height );
}

void Sys_MakeWindowActive( SysWindow *windowPtr ) {
	SDL_GL_MakeCurrent( windowPtr->sdlWindowPtr, windowPtr->sdlGLContext );
}

SysWindow *Sys_CreateWindow( const char *title, int width, int height ) {
	SDL_Window *sdlWindowPtr = SDL_CreateWindow( title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
	if ( sdlWindowPtr == NULL ) {
		PrintError( "Failed to create window!\nSDL: %s\n", SDL_GetError() );
	}

	SDL_GLContext sdlGLContext = SDL_GL_CreateContext( sdlWindowPtr );
	if ( sdlGLContext == NULL ) {
		SDL_DestroyWindow( sdlWindowPtr );
		PrintWarn( "Failed to create OpenGL context!\nSDL: %s\n", SDL_GetError() );
		return NULL;
	}

	SysWindow *window = calloc( 1, sizeof( SysWindow ) );
	window->sdlWindowPtr = sdlWindowPtr;
	window->sdlGLContext = sdlGLContext;

	Sys_MakeWindowActive( window );

	return window;
}

void Sys_DestroyWindow( SysWindow *windowPtr ) {
	if ( windowPtr == NULL ) {
		return;
	}

	if ( windowPtr->sdlGLContext != NULL ) {
		SDL_GL_DeleteContext( windowPtr->sdlGLContext );
	}

	if ( windowPtr->sdlWindowPtr != NULL ) {
		SDL_DestroyWindow( windowPtr->sdlWindowPtr );
	}

	free( windowPtr );
}

void Sys_SwapWindow( SysWindow *windowPtr ) {
	SDL_GL_SwapWindow( windowPtr->sdlWindowPtr );
}

/****************************************
 * INPUT MANAGEMENT
 ****************************************/

static bool buttonStates[ MAX_BUTTON_INPUTS ];
bool Sys_GetButtonState( InputButton inputIndex ) {
	if ( inputIndex >= MAX_BUTTON_INPUTS ) {
		return false;
	}
	return buttonStates[ inputIndex ];
}
static bool keyStates[ MAX_KEY_INPUTS ];
bool Sys_GetKeyState( int keyIndex ) {
	if ( keyIndex >= MAX_KEY_INPUTS ) {
		return false;
	}
	return keyStates[ keyIndex ];
}

static int Sys_TranslateSDLKeyInput( int key ) {
	if ( key < 128 ) {
		return key;
	}

	switch ( key ) {
		default:
			return KEY_INVALID;
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
			g_engine.Shutdown();
			break;
	}
}

static void Sys_HandleKeyboardEvent( int key, bool isDown ) {
	key = Sys_TranslateSDLKeyInput( key );
	if ( key == KEY_INVALID ) {
		PrintWarn( "Unhandled key, %d\n", key );
		return;
	}

	keyStates[ key ] = isDown;
	g_engine.KeyboardEvent( key, isDown );
}

/****************************************
 * TIMER MANAGEMENT
 ****************************************/

static SDL_TimerID timer = 0;
static unsigned int Sys_TimerCallback( unsigned int interval, void *param ) {
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

/**
 * Return whether or not we consider the platform
 * to have a keyboard.
 */
static bool Sys_HasKeyboard( void ) {
#if defined( __ANDROID__ )
	return false;
#else
	return true;
#endif
}

void Sys_Shutdown( void ) {
	exit( EXIT_SUCCESS );
}

int Sys_Init( int argc, char **argv ) {
	/* if we're debugging on Win32 platforms, invoke the console */
#if defined( _WIN32 )
	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );
#endif

	/* setup the engine interface */

	printf( "Setting up engine interface\n" );

#if defined( __amd64__ ) || defined( __amd64 ) || defined( _M_AMD64 ) || defined( __x86_64__ ) || defined( __x86_64 )
	const char *engineLibPath = "./OSEngine_x64";
#else
	const char *engineLibPath = "./OSEngine_x86";
#endif

	dllEnginePtr = plLoadLibrary( engineLibPath, true );
	if ( dllEnginePtr == NULL ) {
		PrintError( "Failed to load engine module, aborting!\nPL: %s\n", plGetError() );
	}

	DllLauncherInterface GetDllInterface = ( DllLauncherInterface ) plGetLibraryProcedure( dllEnginePtr, "GetDllInterface" );
	if ( GetDllInterface == NULL ) {
		PrintError( "Failed to fetch \"" INTERFACE_PROCEDURE "\" from engine module, aborting!\nPL: %s\n", plGetError() );
	}

	SystemInterface systemInterface = {
	        .Shutdown = Sys_Shutdown,
	        .DisplayMessageBox = Sys_DisplayMessageBox,
	        .CreateWindow = Sys_CreateWindow,
	        .DestroyWindow = Sys_DestroyWindow,
	        .GetWindowSize = Sys_GetWindowSize,
	        .MakeWindowActive = Sys_MakeWindowActive,
	        .SwapWindow = Sys_SwapWindow,
	        .GetButtonState = Sys_GetButtonState,
	        .GetKeyState = Sys_GetKeyState,
	        .HasKeyboard = Sys_HasKeyboard,
	};

	/* initialize the interface */
	GetDllInterface( BASE_INTERFACE_VERSION, &systemInterface, &g_engine );

	/* and now setup sdl */

	if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 ) {
		PrintError( "Failed to initialize SDL2!\nSDL: %s\n", SDL_GetError() );
	}

	/* setup our timers, in this case we're just setting up our tick */
	timer = SDL_AddTimer( TICK_RATE, Sys_TimerCallback, NULL );

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

	if ( !g_engine.Initialize( argc, argv ) ) {
		PrintError( "Failed to initialize engine!\nCheck debug logs.\n" );
	}

	while ( g_engine.IsRunning() ) {
		SDL_Event event;
		while ( SDL_PollEvent( &event ) ) {
			switch ( event.type ) {
				case SDL_USEREVENT:
					g_engine.Tick();
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					Sys_HandleKeyboardEvent( event.key.keysym.sym, ( event.type == SDL_KEYDOWN ) );
					break;
			}
		}

		g_engine.Display();
	}

	g_engine.Shutdown();

	return EXIT_SUCCESS;
}

int main( int argc, char **argv ) {
	return Sys_Init( argc, argv );
}
