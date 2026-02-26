// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/* map everything out to controller-style input
 * even if the user isn't necessarily using a controller
 */
typedef enum ApeInputButton
{
	APE_INPUT_INVALID,

	APE_INPUT_UP,
	APE_INPUT_DOWN,
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

	APE_MAX_BUTTON_INPUTS
} ApeInputButton;

typedef enum ApeInputMouseButton
{
	APE_INPUT_MOUSE_BUTTON_LEFT,
	APE_INPUT_MOUSE_BUTTON_RIGHT,
	APE_INPUT_MOUSE_BUTTON_MIDDLE,

	APE_MAX_INPUT_MOUSE_BUTTONS
} ApeInputMouseButton;

typedef enum ApeInputKey
{
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
	APE_INPUT_KEY_ESCAPE,

	APE_INPUT_KEY_UP,
	APE_INPUT_KEY_DOWN,
	APE_INPUT_KEY_LEFT,
	APE_INPUT_KEY_RIGHT,

	KEY_LEFT_CTRL,
	KEY_RIGHT_CTRL,
	KEY_LEFT_SHIFT,
	KEY_RIGHT_SHIFT,
	KEY_LEFT_ALT,
	KEY_RIGHT_ALT,

	APE_MAX_KEY_INPUTS
} ApeInputKey;

typedef enum ApeInputState
{
	APE_INPUT_STATE_NONE = 0, /* key has no state */

	QM_OS_BIT_FLAG( APE_INPUT_STATE_DOWN, 1 ),     /* key is still down */
	QM_OS_BIT_FLAG( APE_INPUT_STATE_PRESSED, 2 ),  /* key has been pressed */
	QM_OS_BIT_FLAG( APE_INPUT_STATE_RELEASED, 3 ), /* key is up */
} ApeInputState;

PL_EXTERN_C

typedef enum SS_Acl_InputDeviceType
{
	CLIENT_INPUT_DEVICE_NONE,
	CLIENT_INPUT_DEVICE_KEYBOARD,
	CLIENT_INPUT_DEVICE_MOUSE,
	CLIENT_INPUT_DEVICE_TOUCH,
	CLIENT_INPUT_DEVICE_CONTROLLER,
} SS_Acl_InputDeviceType;

// Controller API

/**
 * Returns the number of available controllers.
 */
unsigned int apeGetNumControllers( void );

/**
 * Returns the button state for the given slot.
 */
ApeInputState ape_client_input_get_button_state( unsigned int slot, ApeInputButton button );

/**
 * Returns the analogue stick state for the given slot.
 */
QmMathVector2f ape_client_input_get_controller_axis_state( unsigned int slot, unsigned int stickNum );

// Mouse
void ape_client_input_get_mouse_position( int *x, int *y );
void ape_client_input_get_mouse_delta( int *x, int *y );

// Actions

typedef void ( *ApeInputActionCallback )( ApeInputState state, const char *id );

void ape_client_input_register_action( const char *id,
                                       ApeInputButton buttons[], unsigned int numDefaultButtons,
                                       ApeInputKey keys[], unsigned int numDefaultKeys,
                                   ApeInputActionCallback actionCallback );

unsigned int ape_input_register_device( SS_Acl_InputDeviceType type );

PL_EXTERN_C_END
