/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <SDL2/SDL.h>

#include "yin.h"
#include "gfx.h"
#include "act.h"
#include "pkg_loader.h"

PLPackage *globalWad = NULL;

typedef struct SysWindow {
	SDL_Window      *sdlWindowPtr;
	SDL_GLContext   *sdlGLContext;
} SysWindow;
static SysWindow *mainWindow = NULL;

static SDL_TimerID timer = 0;
static unsigned int numTicks = 0;

static EngineInterface engineInterface;
static LaunchMode launchMode = LAUNCH_MODE_DEFAULT;

LaunchMode Sys_GetLaunchMode( void ) {
	return launchMode;
}

SysWindow *Sys_GetMainWindow( void ) {
	return mainWindow;
}

void Sys_GetWindowSize( SysWindow *windowPtr, int *width, int *height ) {
	SDL_GL_GetDrawableSize( windowPtr->sdlWindowPtr, width, height );
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

	SysWindow *window = Sys_AllocateMemory( 1, sizeof( SysWindow ) );
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

void Sys_MakeWindowActive( SysWindow *windowPtr ) {
	SDL_GL_MakeCurrent( windowPtr->sdlWindowPtr, windowPtr->sdlGLContext );
}

void *Sys_AllocateMemory( size_t num, size_t size ) {
	void *mem = calloc( num, size );
	if( mem == NULL ) {
		PrintError( "Failed to allocate %d bytes!\n", num * size );
	}

	return mem;
}

/* wrapper for malloc */
static void *Sys_malloc( size_t size ) { return Sys_AllocateMemory( 1, size ); }

static void Sys_Close( void ) {
	PrintMsg( "Shutting down...\n" );

	engineInterface.Shutdown();

	Act_Shutdown();
	Gfx_Shutdown();

	exit( EXIT_SUCCESS );
}

static void Sys_Display( void ) {
	engineInterface.Display();
}

void Sys_SwapWindow( SysWindow *windowPtr ) {
	SDL_GL_SwapWindow( mainWindow->sdlWindowPtr );
}

static unsigned char Sys_TranslateSDLButtonInput( unsigned int key ) {
	switch( key ) {
		default: return YIN_INPUT_INVALID;
		case 'w': return YIN_INPUT_UP;
		case 's': return YIN_INPUT_DOWN;
		case 'a': return YIN_INPUT_LEFT;
		case 'd': return YIN_INPUT_RIGHT;
		//case GLUT_KEY_SHIFT_L: return YIN_INPUT_LEFT_STICK;
		//case 27: return YIN_INPUT_START; /* escape */
		case ' ': return YIN_INPUT_A;

		case 'z': return YIN_INPUT_LEFT_STICK;

		case SDLK_UP:       return YIN_INPUT_UP;
		case SDLK_DOWN:     return YIN_INPUT_DOWN;
		case SDLK_LEFT:     return YIN_INPUT_LEFT;
		case SDLK_RIGHT:    return YIN_INPUT_RIGHT;

		/* temp temp temp */
		case SDLK_ESCAPE:
			Sys_Close();
			break;
	}

	return key;
}

static bool buttonStates[ MAX_BUTTON_INPUTS ];
static bool keyStates[ 256 ];

bool Sys_GetButtonState( InputButton inputIndex ) { return buttonStates[ inputIndex ]; }
bool Sys_GetKeyState( unsigned char keyIndex ) { return keyStates[ keyIndex ]; }

static void Sys_Keyboard( unsigned char key, int x, int y ) {
	u_unused( x );
	u_unused( y );

	keyStates[ key ] = true;

	/* see if we can translate it to a button */
	unsigned char button = Sys_TranslateSDLButtonInput( key );
	if ( button == YIN_INPUT_INVALID ) {
		return;
	}

	buttonStates[ button ] = true;
}

static void Sys_KeyboardUp( unsigned char key, int x, int y ) {
	u_unused( x );
	u_unused( y );

	keyStates[ key ] = false;

	unsigned char button = Sys_TranslateSDLButtonInput( key );
	if ( button == YIN_INPUT_INVALID ) {
		return;
	}

	buttonStates[ button ] = false;
}

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

unsigned int Sys_GetNumTicks( void ) {
	return numTicks;
}

#if defined( _MSC_VER )
__declspec(noreturn)
#else
_Noreturn
#endif
void Sys_Init( int argc, char **argv ) {
	pl_calloc = Sys_AllocateMemory;
	pl_malloc = Sys_malloc;

	/* initialize the platform library */
	plInitialize( argc, argv );
	plInitializeSubSystems( PL_SUBSYSTEM_GRAPHICS | PL_SUBSYSTEM_IO | PL_SUBSYSTEM_IMAGE );

	plSetupLogOutput( "log.txt" );
	plSetupLogLevel( LOG_LEVEL_ERROR, "error", PL_COLOUR_RED, true );
	plSetupLogLevel( LOG_LEVEL_WARN, "warning", PL_COLOUR_ORANGE, true );
	plSetupLogLevel( LOG_LEVEL_INFO, NULL, PL_COLOUR_WHITE, true );

	PrintMsg( "Yin Engine, Copyright (C) 2020 OldTimes Software\n" );

	plRegisterStandardPackageLoaders();
	plRegisterPackageLoader( "pkg", Pkg_LoadPackage );
	plRegisterPackageLoader( "map", Pkg_LoadPackage );

	PrintMsg( "Mounting VFS locations...\n" );

	/* mount all the dirs and packages we need */
	plMountLocation( plGetWorkingDirectory() );
	const char *rPackages[]={
			"BaseShaders.pkg",
			"BaseTextures.pkg",
	};
	for ( unsigned int i = 0; i < plArrayElements( rPackages ); ++i ) {
		if ( plMountLocation( rPackages[ i ] ) == NULL ) {
			PrintError( "Failed to mount required package \"%s\"!\nPL: %s\n", rPackages[ i ], plGetError() );
		}
	}

	/* ensure our wad is available */
	globalWad = plLoadPackage( YIN_GLOBAL_WAD );
	if( globalWad == NULL ) {
		PrintError( "Failed to load \"" YIN_GLOBAL_WAD "\"!\nPL: %s\n", plGetError() );
	}

	if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 ) {
		PrintError( "Failed to initialize SDL2!\nSDL: %s\n", SDL_GetError() );
	}

	if( plHasCommandLineArgument( "editor" ) ) {
		launchMode = LAUNCH_MODE_EDITOR;
	}

	/* setup our timers, in this case we're just setting up our tick */
	timer = SDL_AddTimer( YIN_TICK_RATE, Sys_TimerCallback, NULL );

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

	mainWindow = Sys_CreateWindow( YIN_WINDOW_TITLE, YIN_WINDOW_WIDTH, YIN_WINDOW_HEIGHT );
	if ( mainWindow == NULL ) {
		PrintError( "Failed to create main window!\n" );
	}

	/* initialize core services */
	Gfx_Initialize();
	Act_Initialize();

	void Editor_SetupInterface( EngineInterface *interface );
	void Game_SetupInterface( EngineInterface *interface );
	switch( launchMode ) {
	case LAUNCH_MODE_EDITOR: 
		Editor_SetupInterface( &engineInterface );
		break;
	case LAUNCH_MODE_DEFAULT:
		Game_SetupInterface( &engineInterface );
		break;
	default:PrintError( "Unhandled launch mode, %d!\n", launchMode );
	}

	engineInterface.Initialize();

	for( ;; ) {
		SDL_Event event;
		while ( SDL_PollEvent( &event )) {
			switch ( event.type ) {
				case SDL_USEREVENT:
					engineInterface.Tick();
					numTicks++;
					break;
				case SDL_KEYUP:
					Sys_KeyboardUp( event.key.keysym.sym, 0, 0 );
					break;
				case SDL_KEYDOWN:
					Sys_Keyboard( event.key.keysym.sym, 0, 0 );
					break;
			}
		}

		Sys_Display();
	}
}

int main( int argc, char **argv ) {
	Sys_Init( argc, argv );
	return EXIT_SUCCESS;
}
