/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <SDL2/SDL.h>

#include "common/common.h"
#include "launcher.h"

OSEngineInterface g_engine;
static PLLibrary *dllEnginePtr;

#define WINDOW_TITLE  "Yin Game Engine"
#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768

void *Sys_calloc( size_t num, size_t size, bool abortOnFail );
void  Sys_free( void *ptr );

/****************************************
 * WINDOW MANAGEMENT
 ****************************************/

static SDL_Window *   sdlWindow = NULL;
static SDL_GLContext *sdlGLContext;

static OSViewport osViewport;

void Sys_DisplayMessageBox( SysMessage messageType, const char *message, ... )
{
	const char *        title;
	SDL_MessageBoxFlags flags;
	switch ( messageType )
	{
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
	char    msgBuf[ 4096 ];
	va_list args;
	va_start( args, message );
	vsnprintf( msgBuf, sizeof( msgBuf ), message, args );
	va_end( args );

	printf( "%s", msgBuf );

	SDL_ShowSimpleMessageBox( flags, title, msgBuf, NULL );
}

static bool Sys_IsDisplayActive( void )
{
	uint32_t flags = SDL_GetWindowFlags( sdlWindow );
	return ( !( flags & SDL_WINDOW_HIDDEN ) && ( flags & SDL_WINDOW_INPUT_FOCUS ) );
}

void Sys_MakeWindowActive( void )
{
	SDL_GL_MakeCurrent( sdlWindow, sdlGLContext );
}

static void Sys_CreateWindow( const char *title, int width, int height )
{
	sdlWindow = SDL_CreateWindow(
	        WINDOW_TITLE,
	        SDL_WINDOWPOS_UNDEFINED,
	        SDL_WINDOWPOS_UNDEFINED,
	        WINDOW_WIDTH, WINDOW_HEIGHT,
	        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
	if ( sdlWindow == NULL )
		PrintError( "Failed to create window!\nSDL: %s\n", SDL_GetError() );

	sdlGLContext = SDL_GL_CreateContext( sdlWindow );
	if ( sdlGLContext == NULL )
	{
		SDL_DestroyWindow( sdlWindow );
		PrintError( "Failed to create OpenGL context!\nSDL: %s\n", SDL_GetError() );
	}

	Sys_MakeWindowActive();
}

static void Sys_DestroyWindow( void )
{
	if ( sdlGLContext != NULL )
		SDL_GL_DeleteContext( sdlGLContext );

	if ( sdlWindow != NULL )
		SDL_DestroyWindow( sdlWindow );
}

static void Sys_SwapWindow( void )
{
	SDL_GL_SwapWindow( sdlWindow );
}

static bool Sys_SetWindowSize( int *width, int *height )
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

/****************************************
 * INPUT MANAGEMENT
 ****************************************/

static OSInputState buttonStates[ MAX_BUTTON_INPUTS ];
OSInputState        Sys_GetButtonState( InputButton buttonIndex )
{
	if ( buttonIndex >= MAX_BUTTON_INPUTS )
		return INPUT_STATE_NONE;

	return buttonStates[ buttonIndex ];
}

static OSInputState keyStates[ MAX_KEY_INPUTS ];
OSInputState        Sys_GetKeyState( int keyIndex )
{
	if ( keyIndex >= MAX_KEY_INPUTS )
		return INPUT_STATE_NONE;

	return keyStates[ keyIndex ];
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
			g_engine.Shutdown();
			break;
	}

	return KEY_INVALID;
}

static void Sys_HandleTextEvent( const char *key )
{
	g_engine.TextEvent( key );
}

static void Sys_HandleKeyboardEvent( int key, bool isDown )
{
	if ( key == KEY_INVALID )
	{
		PrintWarn( "Unhandled key, %d\n", key );
		return;
	}

	/* figure out what state the key is in now */
	if ( keyStates[ key ] == INPUT_STATE_PRESSED && isDown )
		keyStates[ key ] = INPUT_STATE_DOWN;
	else if ( keyStates[ key ] == INPUT_STATE_PRESSED && !isDown )
		keyStates[ key ] = INPUT_STATE_RELEASED;

	switch ( keyStates[ key ] )
	{
		case INPUT_STATE_PRESSED:
			keyStates[ key ] = isDown ? INPUT_STATE_DOWN : INPUT_STATE_RELEASED;
			break;
		case INPUT_STATE_RELEASED:
			keyStates[ key ] = isDown ? INPUT_STATE_PRESSED : INPUT_STATE_NONE;
			break;
		case INPUT_STATE_DOWN:
			keyStates[ key ] = isDown ? INPUT_STATE_DOWN : INPUT_STATE_RELEASED;
			break;
		case INPUT_STATE_NONE:
			keyStates[ key ] = isDown ? INPUT_STATE_PRESSED : INPUT_STATE_RELEASED;
			break;
	}

	g_engine.KeyboardEvent( key, keyStates[ key ] );
}

/****************************************
 * TIMER MANAGEMENT
 ****************************************/

static SDL_TimerID  sdlTimer = 0;
static unsigned int Sys_TimerCallback( unsigned int interval, void *param )
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
 * MEMORY MANAGEMENT
 ****************************************/

//#define DEBUG_MEMORY

#ifdef DEBUG_MEMORY
/* description of what's been allocated, for debugging */
typedef struct AllocHeader
{
	char   id[ 32 ]; /* unique identifier */
	size_t length;   /* allocated size in bytes */
} AllocHeader;
static size_t totalRAMUsage = 0;
#endif

/* wrapper for calloc */
void *Sys_calloc( size_t num, size_t size, bool abortOnFail )
{
	size_t totalSize = num * size;

#ifdef DEBUG_MEMORY
	totalSize += sizeof( AllocHeader );
#endif

	char *buf = calloc( 1, totalSize );
	if ( buf == NULL )
	{
		if ( abortOnFail )
			PrintError( "Failed to allocate %d bytes!\n", totalSize );

		PrintWarn( "Failed to allocate %d bytes!\n", totalSize );
		return NULL;
	}

#ifdef DEBUG_MEMORY
	AllocHeader *header = ( AllocHeader * ) buf;
	PlGenerateUniqueIdentifier( header->id, sizeof( header->id ) - 1 );
	header->length = totalSize;

	totalRAMUsage += totalSize;

	printf( "ALLOC: " COM_FMT_uint64 " bytes (%s)\t\t | TOTAL: " COM_FMT_double "mb\n",
	        header->length, header->id,
	        PlBytesToMegabytes( totalRAMUsage ) );

	buf += sizeof( AllocHeader );
#endif

	return buf;
}

/* wrapper for malloc */
void *Sys_malloc( size_t size, bool abortOnFail )
{
	return Sys_calloc( 1, size, abortOnFail );
}

/* wrapper for realloc */
void *Sys_realloc( void *ptr, size_t newSize, bool abortOnFail )
{
	char *buf = ptr;

#ifdef DEBUG_MEMORY
	if ( buf == NULL )
		return Sys_malloc( newSize, abortOnFail );

	buf -= sizeof( AllocHeader );
	newSize += sizeof( AllocHeader ); /* maybe... ? */
#endif

	buf = realloc( buf, newSize );
	if ( buf == NULL )
	{
		if ( abortOnFail )
			PrintError( "Failed to allocate %d bytes!\n", newSize );

		PrintWarn( "Failed to allocate %d bytes!\n", newSize );
		return NULL;
	}

#ifdef DEBUG_MEMORY
	AllocHeader *header    = ( AllocHeader * ) buf;
	size_t       oldLength = header->length;
	header->length         = newSize;

	totalRAMUsage -= oldLength;
	totalRAMUsage += newSize;

	printf( "REALLOC: " COM_FMT_uint64 " bytes (%s)\t\t | TOTAL: " COM_FMT_double "mb\n",
	        header->length, header->id,
	        PlBytesToMegabytes( totalRAMUsage ) );

	buf += sizeof( AllocHeader );
#endif

	return buf;
}

void Sys_free( void *ptr )
{
	char *buf = ptr;

#ifdef DEBUG_MEMORY
	if ( buf != NULL )
	{
		buf -= sizeof( AllocHeader );
		AllocHeader *header = ( AllocHeader * ) buf;
		u_assert( header != NULL );
		if ( header != NULL )
		{
			totalRAMUsage -= header->length;

			printf( "FREE: " COM_FMT_uint64 " bytes (%s)\t\t | TOTAL: " COM_FMT_double "mb\n",
			        header->length, header->id,
			        PlBytesToMegabytes( totalRAMUsage ) );
		}
	}
#endif

	free( buf );
}

/* wrappers for platform lib */
void *Sys_WMAlloc( size_t size ) { return Sys_malloc( size, true ); }
void *Sys_WCAlloc( size_t num, size_t size ) { return Sys_calloc( num, size, true ); }
void *Sys_WReAlloc( void *ptr, size_t newSize ) { return Sys_realloc( ptr, newSize, true ); }

/****************************************
 * INITIALIZATION
 ****************************************/

void Sys_Shutdown( void )
{
	if ( sdlTimer != 0 )
		SDL_RemoveTimer( sdlTimer );

	exit( EXIT_SUCCESS );
}

/**
 * Load in the DLL interface for the engine.
 */
static void Sys_SetupEngineInterface( void )
{
	Print( "Setting up engine interface\n" );

	dllEnginePtr = PlLoadLibrary( "./engine", true );
	if ( dllEnginePtr == NULL )
		PrintError( "Failed to load engine module, aborting!\nPL: %s\n", PlGetError() );

	DllEngineInterface GetDllInterface = ( DllEngineInterface ) PlGetLibraryProcedure( dllEnginePtr, "GetDllInterface" );
	if ( GetDllInterface == NULL )
		PrintError( "Failed to fetch \"" INTERFACE_PROCEDURE "\" from engine module, aborting!\nPL: %s\n", PlGetError() );

	static OSSystemInterface systemInterface = {
	        .version = { ENGINE_INTERFACE_VERSION_MAJOR, ENGINE_INTERFACE_VERSION_MINOR },

	        .viewport       = &osViewport,
	        .SetDisplaySize = Sys_SetWindowSize,

	        .Shutdown       = Sys_Shutdown,
	        .GetButtonState = Sys_GetButtonState,
	        .GetKeyState    = Sys_GetKeyState,

	        .GetPerformanceCounter   = SDL_GetPerformanceCounter,
	        .GetPerformanceFrequency = SDL_GetPerformanceFrequency,

	        .CAlloc  = Sys_calloc,
	        .MAlloc  = Sys_malloc,
	        .ReAlloc = Sys_realloc,
	        .Free    = Sys_free,
	};

	/* initialize the interface */
	g_engine = *GetDllInterface( ENGINE_INTERFACE_VERSION, &systemInterface );
	if ( g_engine.version[ VERSION_MAJOR ] != ENGINE_INTERFACE_VERSION_MAJOR )
		PrintWarn( "Unexpected major interface version (%d vs %d)!\n", g_engine.version[ VERSION_MAJOR ], ENGINE_INTERFACE_VERSION );
}

/****************************************
 ****************************************/

int launcherLog;

int Sys_Init( int argc, char **argv )
{
#if defined( _WIN32 )
	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );
#endif

	pl_calloc  = Sys_WCAlloc;
	pl_malloc  = Sys_WMAlloc;
	pl_realloc = Sys_WReAlloc;
	pl_free    = Sys_free;

	/* initialize the platform library */
	PlInitialize( argc, argv );
	PlInitializeSubSystems( PL_SUBSYSTEM_IO );

	if ( PlHasCommandLineArgument( "-log" ) )
	{
		const char *path = PlGetCommandLineArgumentValue( "-log" );
		if ( path == NULL )
			path = "log.txt";

		PlSetupLogOutput( path );
	}

	launcherLog = PlAddLogLevel( "launcher", PL_COLOUR_WHITE, true );
	PlLogMessage( launcherLog, "Log output initialized!\n" );

	ComLib_Initialize();

	if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 )
		PrintError( "Failed to initialize SDL2!\nSDL: %s\n", SDL_GetError() );

	/* setup our timers, in this case we're just setting up our tick */
	sdlTimer = SDL_AddTimer( TICK_RATE, Sys_TimerCallback, NULL );

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

	Sys_CreateWindow( WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT );

	SDL_GL_GetDrawableSize( sdlWindow, &osViewport.w, &osViewport.h );

	PlgInitializeGraphics();
	PlgScanForDrivers( "./" );

	Sys_SetupEngineInterface();

	if ( !g_engine.Initialize( argc, argv ) )
		PrintError( "Failed to initialize engine!\nCheck debug logs.\n" );

	SDL_StartTextInput();

	while ( g_engine.IsRunning() )
	{
		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			switch ( event.type )
			{
				case SDL_USEREVENT:
					g_engine.Tick();

#if 0
					/* hack: need to update state to down if pressed */
					for ( unsigned int i = 0; i < MAX_KEY_INPUTS; ++i )
					{
						if ( keyStates[ i ] != INPUT_STATE_PRESSED )
							continue;

						Sys_HandleKeyboardEvent( i, true );
					}
#endif
					break;

				case SDL_TEXTINPUT:
					Sys_HandleTextEvent( event.text.text );
					break;

				case SDL_KEYDOWN:
				case SDL_KEYUP:
					Sys_HandleKeyboardEvent( Sys_TranslateSDLKeyInput( event.key.keysym.sym ), ( event.type == SDL_KEYDOWN ) );
					break;

				case SDL_WINDOWEVENT:
				{
					if ( sdlWindow != NULL && !( event.window.windowID == SDL_GetWindowID( sdlWindow ) ) )
						break;

					switch ( event.window.event )
					{
						case SDL_WINDOWEVENT_RESIZED:
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							SDL_GL_GetDrawableSize( sdlWindow, &osViewport.w, &osViewport.h );
							break;
					}
				}
			}
		}

		g_engine.Display();

		SDL_GL_SwapWindow( sdlWindow );
	}

	SDL_StopTextInput();

	g_engine.Shutdown();

	return EXIT_SUCCESS;
}

int main( int argc, char **argv )
{
	return Sys_Init( argc, argv );
}
