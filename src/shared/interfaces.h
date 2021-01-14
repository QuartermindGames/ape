/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

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

typedef enum InputKey {
	KEY_INVALID = -1,

	KEY_CAPSLOCK = 128,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,

	KEY_PRINTSCREEN,
	KEY_SCROLLLOCK,
	KEY_PAUSE,
	KEY_INSERT,
	KEY_HOME,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_DELETE,
	KEY_END,
	KEY_TAB,

	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,

	KEY_LEFT_CTRL,
	KEY_RIGHT_CTRL,
	KEY_LEFT_SHIFT,
	KEY_RIGHT_SHIFT,
	KEY_LEFT_ALT,
	KEY_RIGHT_ALT,

	MAX_KEY_INPUTS
} InputKey;

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
	bool ( *GetKeyState )( int keyIndex );
	bool ( *HasKeyboard )( void );

	void ( *Shutdown )( void );
} SystemInterface;
extern SystemInterface g_system;

typedef struct GameInterface {
	unsigned int version;
} GameInterface;
extern GameInterface globalGame;

typedef struct EngineInterface {
	bool ( *Initialize )( int argc, char **argv );
	void ( *Tick )( void );
	void ( *Display )( void );
	void ( *KeyboardEvent )( int key, bool isDown );
	void ( *Shutdown )( void );

	bool ( *IsRunning )( void );

	unsigned int ( *GetNumTicks )( void );
} EngineInterface;
extern EngineInterface g_engine;

#define BASE_INTERFACE_VERSION 1

#define INTERFACE_PROCEDURE "GetDllInterface"
typedef bool ( *DllLauncherInterface )( uint32_t version, const SystemInterface *sysIn, EngineInterface *engOut );
typedef void ( *DllGameInterface )( uint32_t version, const EngineInterface *engIn );

#define TICK_RATE 1000 / 60 /* ms */

#define u_unused( a ) ( void ) ( ( a ) )

#if defined( _DEBUG )
#include <assert.h>
#define u_assert( A ) assert( ( A ) )
#else
#define u_assert( A )
#endif
