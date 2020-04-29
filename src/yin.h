/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/platform.h>
#include <PL/platform_console.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_package.h>
#include <PL/pl_graphics.h>
#include <PL/pl_graphics_camera.h>

#include <assert.h>

#define YIN_DISPLAY_WIDTH  640
#define YIN_DISPLAY_HEIGHT 480

#define YIN_WINDOW_WIDTH  640
#define YIN_WINDOW_HEIGHT 480

#define YIN_WINDOW_TITLE "Yin Engine"

#define YIN_TICK_RATE 1000 / 60 /* ms */

#define u_unused( a ) ( void )( ( a ) )

typedef enum LaunchMode {
	LAUNCH_MODE_DEFAULT,	/* aka, game mode */
	LAUNCH_MODE_EDITOR,		/* edit mode */
} LaunchMode;

/* map everything out to controller-style input
 * even if the user isn't necessarily using a controller
 */
typedef enum InputButton {
	YIN_INPUT_INVALID,

	YIN_INPUT_UP,
	YIN_INPUT_DOWN,
	YIN_INPUT_LEFT,
	YIN_INPUT_RIGHT,

	YIN_INPUT_LEFT_STICK,
	YIN_INPUT_RIGHT_STICK,

	YIN_INPUT_START,

	YIN_INPUT_A,
	YIN_INPUT_B,
	YIN_INPUT_X,
	YIN_INPUT_Y,

	YIN_INPUT_LB,
	YIN_INPUT_LT,
	YIN_INPUT_RB,
	YIN_INPUT_RT,

	MAX_BUTTON_INPUTS
} InputButton;

enum {
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_INFO,
};

//#define DEBUG_CAM
//#define DEBUG_WALL_NORMALS

typedef struct SysWindow SysWindow;

#define PrintError( ... ) plLogMessage( LOG_LEVEL_ERROR, __VA_ARGS__ ); exit( EXIT_FAILURE )
#define PrintWarn( ... )  plLogMessage( LOG_LEVEL_WARN, __VA_ARGS__ )
#define PrintMsg( ... )   plLogMessage( LOG_LEVEL_INFO, __VA_ARGS__ )

extern PLPackage *globalWad;
#define YIN_GLOBAL_WAD "yin.wad"

/* windowing API */
SysWindow *Sys_CreateWindow( const char *title, int width, int height );
void Sys_DestroyWindow( SysWindow *windowPtr );
void Sys_MakeWindowActive( SysWindow *windowPtr );

bool Sys_GetInputState( InputButton inputIndex );
LaunchMode Sys_GetLaunchMode( void );

void *Sys_AllocateMemory( size_t num, size_t size );

unsigned int Sys_GetNumTicks( void );
