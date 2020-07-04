/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * */

#include <SDL2/SDL.h>

#include "launcher.h"

EngineInterface g_engine;
static PLLibrary *dllEnginePtr;

#define PrintWarn( ... )    Sys_DisplayMessageBox( SYS_MESSAGE_WARNING, __VA_ARGS__ )
#define PrintError( ... )   Sys_DisplayMessageBox( SYS_MESSAGE_ERROR, __VA_ARGS__ ); exit( 0 )

/****************************************
 * WINDOW MANAGEMENT
 ****************************************/

void Sys_DisplayMessageBox( SysMessage messageType, const char *message, ... ) {
	const char *title;
	SDL_MessageBoxFlags flags;
	switch( messageType ) {
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
	vsnprintf( msgBuf, sizeof( msgBuf ), message, args);
	va_end( args );

	printf( "%s", msgBuf );

	SDL_ShowSimpleMessageBox( flags, title, msgBuf, NULL );
}

typedef struct SysWindow {
	SDL_Window      *sdlWindowPtr;
	SDL_GLContext   *sdlGLContext;
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
static bool keyStates[ 256 ];

bool Sys_GetButtonState( InputButton inputIndex ) { return buttonStates[ inputIndex ]; }
bool Sys_GetKeyState( unsigned char keyIndex ) { return keyStates[ keyIndex ]; }

static unsigned char Sys_TranslateSDLButtonInput( unsigned int key ) {
	switch( key ) {
		default: return INPUT_INVALID;
		case 'w': return INPUT_UP;
		case 's': return INPUT_DOWN;
		case 'a': return INPUT_LEFT;
		case 'd': return INPUT_RIGHT;
		case ' ': return INPUT_A;
		case 'z': return INPUT_LEFT_STICK;

		case SDLK_UP:       return INPUT_UP;
		case SDLK_DOWN:     return INPUT_DOWN;
		case SDLK_LEFT:     return INPUT_LEFT;
		case SDLK_RIGHT:    return INPUT_RIGHT;

			/* temp temp temp */
		case SDLK_ESCAPE:
			g_engine.Shutdown();
			break;
	}

	return key;
}

static void Sys_Keyboard( unsigned char key, int x, int y ) {
	u_unused( x );
	u_unused( y );

	keyStates[ key ] = true;

	/* see if we can translate it to a button */
	unsigned char button = Sys_TranslateSDLButtonInput( key );
	if ( button == INPUT_INVALID ) {
		return;
	}

	buttonStates[ button ] = true;
}

static void Sys_KeyboardUp( unsigned char key, int x, int y ) {
	u_unused( x );
	u_unused( y );

	keyStates[ key ] = false;

	unsigned char button = Sys_TranslateSDLButtonInput( key );
	if ( button == INPUT_INVALID ) {
		return;
	}

	buttonStates[ button ] = false;
}

/****************************************
 * MEMORY MANAGEMENT
 ****************************************/

void *Sys_calloc( size_t num, size_t size ) {
	void *mem = calloc( num, size );
	if( mem == NULL ) {
		PrintError( "Failed to allocate %d bytes!\n", num * size );
	}

	return mem;
}

/* wrapper for malloc */
static void *Sys_malloc( size_t size ) {
	return Sys_calloc( 1, size );
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

void Sys_Shutdown( void ) {
	exit( EXIT_SUCCESS );
}

int Sys_Init( int argc, char **argv ) {
	/* if we're debugging on Win32 platforms, invoke the console */
#if defined( _WIN32 ) && defined( _DEBUG )
	AllocConsole();

	FILE *dummyFilePtr;
	freopen_s( &dummyFilePtr, "CONIN$", "r", stdin );
	freopen_s( &dummyFilePtr, "CONOUT$", "w", stderr );
	freopen_s( &dummyFilePtr, "CONOUT$", "w", stdout );
#endif

	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );

	/* setup the engine interface */

	printf( "Setting up engine interface\n" );

	dllEnginePtr = plLoadLibrary( "libengine", true );
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
		.calloc = Sys_calloc,
		.malloc = Sys_malloc,
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

	while( g_engine.IsRunning() ) {
		SDL_Event event;
		while ( SDL_PollEvent( &event )) {
			switch ( event.type ) {
				case SDL_USEREVENT:
					g_engine.Tick();
					break;
				case SDL_KEYUP:
					Sys_KeyboardUp( event.key.keysym.sym, 0, 0 );
					break;
				case SDL_KEYDOWN:
					Sys_Keyboard( event.key.keysym.sym, 0, 0 );
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
