/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * */

/* map everything out to controller-style input
 * even if the user isn't necessarily using a controller
 */
typedef enum InputButton {
	INPUT_INVALID,

	INPUT_UP,
	INPUT_DOWN,
	INPUT_LEFT,
	INPUT_RIGHT,

	INPUT_LEFT_STICK,
	INPUT_RIGHT_STICK,

	INPUT_START,

	INPUT_A,
	INPUT_B,
	INPUT_X,
	INPUT_Y,

	INPUT_LB,
	INPUT_LT,
	INPUT_RB,
	INPUT_RT,

	MAX_BUTTON_INPUTS
} InputButton;

typedef enum SysMessage {
	SYS_MESSAGE_ERROR,
	SYS_MESSAGE_WARNING,
	SYS_MESSAGE_INFO,
} SysMessage;

typedef struct SysWindow SysWindow;

typedef struct SystemInterface {
	/* windowing */
	void ( *DisplayMessageBox )( SysMessage messageType, const char *message, ... );
	SysWindow *( *CreateWindow )( const char *title, int width, int height );
	SysWindow *( *GetMainWindow )( void );
	void ( *DestroyWindow )( SysWindow *windowPtr );
	void ( *MakeWindowActive )( SysWindow *windowPtr );
	void ( *SwapWindow )( SysWindow *windowPtr );
	void ( *GetWindowSize )( SysWindow *windowPtr, int *width, int *height );

	/* input */
	bool ( *GetButtonState )( InputButton inputIndex );
	bool ( *GetKeyState )( unsigned char keyIndex );

	/* memory */
	void *( *calloc )( size_t num, size_t size );
	void *( *malloc )( size_t size );
	void *( *realloc )( void *ptr, size_t newSize );

	void ( *Shutdown )( void );
} SystemInterface;
extern SystemInterface g_system;

typedef struct EngineInterface {
	bool ( *Initialize )( int argc, char **argv );
	void ( *Tick )( void );
	void ( *Display )( void );
	void ( *Keyboard )( unsigned char key, bool isDown );
	void ( *Shutdown )( void );

	bool ( *IsRunning )( void );

	unsigned int ( *GetNumTicks )( void );
} EngineInterface;
extern EngineInterface g_engine;

#define ENGINE_INTERFACE_VERSION    sizeof( EngineInterface )
#define LAUNCHER_INTERFACE_VERSION  sizeof( LauncherInteface )
#define BASE_INTERFACE_VERSION      sizeof( EngineInterface ) + sizeof( SystemInterface )

#define INTERFACE_PROCEDURE "GetDllInterface"
typedef bool ( *DllLauncherInterface )( uint32_t version, const SystemInterface *sysIn, EngineInterface *engOut );
typedef void ( *DllPluginInterface )( uint32_t version, EngineInterface *engIn );

#define TICK_RATE 1000 / 60 /* ms */

#define u_unused( a ) ( void )( ( a ) )

#if defined( _DEBUG )
#   include <assert.h>
#   define u_assert( A ) assert( ( A ) )
#else
#   define u_assert( A )
#endif
