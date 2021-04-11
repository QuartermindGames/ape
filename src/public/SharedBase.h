/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <PL/platform.h>
#include <PL/platform_math.h>

#define YIN_GLOBAL_WAD "base.pkg"

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

	KEY_BACKSPACE = 8,
	KEY_TAB = 9,
	KEY_ENTER = 13,

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

enum {
	INPUT_STATE_NONE,     /* key has no state */
	INPUT_STATE_DOWN,     /* key has been pressed */
	INPUT_STATE_PRESSING, /* key is still down */
	INPUT_STATE_UP,       /* key is up */
};

typedef enum SysMessage {
	SYS_MESSAGE_ERROR,
	SYS_MESSAGE_WARNING,
	SYS_MESSAGE_INFO,
} SysMessage;

typedef struct SysWindow SysWindow;

#include "SharedInterfaces.h"

#define TICK_RATE 1000 / 60 /* ms */

#define u_unused( a ) ( void ) ( ( a ) )

#if !defined( NDEBUG )
#include <assert.h>
#define u_assert( A ) assert( ( A ) )
#else
#define u_assert( A )
#endif
