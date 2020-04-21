/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "gfx.h"
#include "act.h"
#include "game.h"
#include "pkg_loader.h"
#include "image.h"

#include <GL/freeglut.h>

PLPackage *globalWad = NULL;

unsigned int numTicks = 0;

void *Sys_AllocateMemory( size_t num, size_t size ) {
	void *mem = calloc( num, size );
	if( mem == NULL ) {
		PrintError( "Failed to allocate %d bytes!\n", num * size );
	}

	return mem;
}

/* wrapper for malloc */
static void *Sys_malloc( size_t size ) {
	return Sys_AllocateMemory( 1, size );
}

static void Sys_Close( void ) {
	Act_Shutdown();
	Gam_Shutdown();
	Gfx_Shutdown();
}

static void Sys_Display( void ) {
	Gfx_Display();

	glutSwapBuffers();
}

static unsigned char Sys_TranslateKeyboardInput( unsigned char key ) {
	switch( key ) {
		default: return YIN_INPUT_INVALID;
		case 'w': return YIN_INPUT_UP;
		case 's': return YIN_INPUT_DOWN;
		case 'a': return YIN_INPUT_LEFT;
		case 'd': return YIN_INPUT_RIGHT;
		case GLUT_KEY_SHIFT_L: return YIN_INPUT_LEFT_STICK;
		case 27: return YIN_INPUT_START; /* escape */
		case ' ': return YIN_INPUT_A;
	}
}

bool keyStates[ MAX_BUTTON_INPUTS ];
bool Sys_GetInputState( InputButton inputIndex ) {
	return keyStates[ inputIndex ];
}

static void Sys_Keyboard( unsigned char key, int x, int y ) {
	u_unused( x );
	u_unused( y );

	Gam_Keyboard( key );

	key = Sys_TranslateKeyboardInput( key );
	if ( key == YIN_INPUT_INVALID ) {
		return;
	}

	keyStates[ key ] = true;
}

static void Sys_KeyboardUp( unsigned char key, int x, int y ) {
	u_unused( x );
	u_unused( y );

	key = Sys_TranslateKeyboardInput( key );
	if ( key == YIN_INPUT_INVALID ) {
		return;
	}

	keyStates[ key ] = false;
}

static void Sys_Reshape( int width, int height ) {
	/* this seems to be broken, at least from what I tested on Windows 10 */
	//glutReshapeWindow( YIN_DISPLAY_WIDTH, YIN_DISPLAY_HEIGHT );
}

static void Sys_Idle( void ) {
	Sys_Display();
}

static void Sys_Tick( int time ) {
	Gam_Tick();

	numTicks++;

	glutTimerFunc( YIN_TICK_RATE, Sys_Tick, 0 );
}

unsigned int Sys_GetNumTicks( void ) {
	return numTicks;
}

void Sys_MountPackageCallback( const char *path, void *userData ) {
	PrintMsg( "Mounting %s as directory\n" );
	plMountLocation( path );
}

int Sys_Init( int argc, char **argv ) {
	pl_calloc = Sys_AllocateMemory;
	pl_malloc = Sys_malloc;

	/* initialize the platform library */
	plInitialize( argc, argv );
	plInitializeSubSystems( PL_SUBSYSTEM_GRAPHICS | PL_SUBSYSTEM_IO | PL_SUBSYSTEM_IMAGE );

	plSetupLogOutput( "log.txt" );
	plSetupLogLevel( LOG_LEVEL_ERROR, "error", PL_COLOUR_RED, true );
	plSetupLogLevel( LOG_LEVEL_WARN, "warning", PL_COLOUR_ORANGE, true );
	plSetupLogLevel( LOG_LEVEL_INFO, NULL, PL_COLOUR_WHITE, true );

	PrintMsg( "Initializing Yin Engine...\n" );

	plRegisterStandardPackageLoaders();
	plRegisterPackageLoader( "pkg", Pkg_LoadPackage );

	PrintMsg( "Mounting VFS locations...\n" );

	/* mount all the dirs and packages we need */
	plMountLocation( plGetWorkingDirectory() );
	const char *rPackages[]={
			"BaseShaders.pkg",
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

	PLImage *image = Image_LoadPackedImage( "Default.gfx" );
	plWriteImage( image, "test.png" );

	glutInitWindowSize( YIN_WINDOW_WIDTH, YIN_WINDOW_HEIGHT );
	glutInitWindowPosition( 256, 256 );
	glutInit( &argc, argv );
	glutInitContextVersion( 3, 2 );
	glutInitContextFlags( GLUT_CORE_PROFILE | GLUT_DEBUG );
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );

	glutCreateWindow( YIN_WINDOW_TITLE );

	Gfx_Initialize();

	glutReshapeFunc( Sys_Reshape );
	glutDisplayFunc( Sys_Display );
	glutKeyboardFunc( Sys_Keyboard );
	glutKeyboardUpFunc( Sys_KeyboardUp );
	glutCloseFunc( Sys_Close );
	glutIdleFunc( Sys_Idle );

	glutTimerFunc( YIN_TICK_RATE, Sys_Tick, 0 );

	Act_Initialize();

	glutMainLoop();

	return EXIT_SUCCESS;
}

#if defined( _WIN32 )

#include <Windows.h>
#include <shellapi.h>

int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
) {
	int numArguments;
	LPWSTR* strArgs = CommandLineToArgvW( (LPCWSTR) lpCmdLine, &numArguments );
	if( strArgs == NULL ) {
		printf( "Failed to get command line arguments from Win32 API!\n" );
		return EXIT_FAILURE;
	}

	return Sys_Init(numArguments, (char **) strArgs);
}

#else

int main( int argc, char **argv ) {
	return Sys_Init( argc, argv );
}

#endif
