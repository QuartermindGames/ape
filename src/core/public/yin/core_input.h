// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

/* map everything out to controller-style input
 * even if the user isn't necessarily using a controller
 */
typedef enum OgeInputButton
{
	YN_CORE_INPUT_INVALID,

	YN_CORE_INPUT_UP,
	YN_CORE_INPUT_DOWN,
	INPUT_LEFT,
	INPUT_RIGHT,

	INPUT_LEFT_STICK,
	INPUT_RIGHT_STICK,

	INPUT_START,
	INPUT_BACK,

	INPUT_A,
	INPUT_B,
	INPUT_X,
	INPUT_Y,

	INPUT_LB,
	INPUT_LT,
	INPUT_RB,
	INPUT_RT,

	YN_CORE_MAX_BUTTON_INPUTS
} OgeInputButton;

typedef enum OgeInputMouseButton
{
	CLIENT_INPUT_MOUSE_BUTTON_LEFT,
	CLIENT_INPUT_MOUSE_BUTTON_RIGHT,
	CLIENT_INPUT_MOUSE_BUTTON_MIDDLE,

	YN_CORE_MAX_INPUT_MOUSE_BUTTONS
} OgeInputMouseButton;

typedef enum OgeInputKey
{
	KEY_INVALID = -1,

	KEY_BACKSPACE = 8,
	KEY_TAB       = 9,
	KEY_ENTER     = 13,

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

	YN_CORE_MAX_KEY_INPUTS
} OgeInputKey;

typedef enum OgeInputState
{
	OGE_INPUT_STATE_NONE,     /* key has no state */
	OGE_INPUT_STATE_PRESSED,  /* key has been pressed */
	OGE_INPUT_STATE_DOWN,     /* key is still down */
	OGE_INPUT_STATE_RELEASED, /* key is up */
} OgeInputState;

PL_EXTERN_C

typedef enum OgeInputDeviceType
{
	CLIENT_INPUT_DEVICE_NONE,
	CLIENT_INPUT_DEVICE_KEYBOARD,
	CLIENT_INPUT_DEVICE_MOUSE,
	CLIENT_INPUT_DEVICE_TOUCH,
	CLIENT_INPUT_DEVICE_CONTROLLER,
} OgeInputDeviceType;

// Controller API

/**
 * Returns the number of available controllers.
 */
unsigned int ogeGetNumControllers( void );

/**
 * Returns the button state for the given slot.
 */
OgeInputState ogeGetButtonStatus( unsigned int slot, OgeInputButton button );

/**
 * Returns the analogue stick state for the given slot.
 */
PLVector2 ogeGetJoystickStatus( unsigned int slot, unsigned int stickNum );

// Mouse
void ogeGetMousePosition( int *x, int *y );
void ogeGetMouseDelta( int *x, int *y );

// Actions

typedef void ( *OgeInputActionCallback )( OgeInputState state, const char *id );

void ogeRegisterInputAction( const char *id,
                             OgeInputButton buttons[], unsigned int numDefaultButtons,
                             OgeInputKey keys[], unsigned int numDefaultKeys,
                             OgeInputActionCallback actionCallback );
OgeInputState ogeGetInputActionState( const char *id );

PL_EXTERN_C_END
