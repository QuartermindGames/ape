// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Client input management.
// Author:  Mark E. Sowden

#include <SDL3/SDL.h>

#include "ape_private.h"
#include "ape_client_input.h"
#include "gui/gui_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static AcmBranch *inputConfig;

static const float DEFAULT_DEADZONE = 0.2f;

#define SERIALISATION_NODE_NAME "input"

typedef struct ApeInputAction
{
	char                   id[ 32 ];
	ApeInputActionCallback callback;

	ApeInputButton buttons[ APE_MAX_BUTTON_INPUTS ];
	unsigned int   numButtonBinds;

	ApeInputKey  keys[ APE_MAX_KEY_INPUTS ];
	unsigned int numKeyBinds;

	QmOsLinkedListNode *node;
} ApeInputAction;

static QmOsLinkedList *actionableList = nullptr;

typedef struct Button
{
	ApeInputState       state;
	QmOsLinkedListNode *activeNode;
} Button;

static struct
{
	int           x, y;
	int           ox, oy;
	int           dx, dy;
	ApeInputState buttons[ APE_MAX_INPUT_MOUSE_BUTTONS ];

	QmMathVector2f wheel, oldWheel;
} inputMouse = {};

static struct
{
	Button          keys[ APE_MAX_KEY_INPUTS ];
	QmOsLinkedList *activeKeyList;
} inputKeyboard = {};

#define CLIENT_INPUT_MAX_CONTROLLERS 4

static struct
{
	bool isActive;

	Button          buttons[ APE_MAX_BUTTON_INPUTS ];
	QmOsLinkedList *activeButtonList;

	QmMathVector2f stickL, stickLOld, stickLDelta;
	QmMathVector2f stickR, stickROld, stickRDelta;
	QmMathVector2f deadzones;

	SDL_Gamepad *sdlGameController;
} inputControllers[ CLIENT_INPUT_MAX_CONTROLLERS ];
static unsigned int numControllers;

static unsigned int get_empty_controller( unsigned int *id )
{
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
	{
		if ( inputControllers[ i ].isActive )
		{
			continue;
		}

		*id = i;
		return i;
	}

	return -1;
}

static void iterate_action( const ApeInputAction *action )
{
	for ( unsigned int i = 0; i < action->numButtonBinds; ++i )
	{
		for ( unsigned int j = 0; j < CLIENT_INPUT_MAX_CONTROLLERS; ++j )
		{
			ApeInputState state = inputControllers[ j ].buttons[ action->buttons[ i ] ].state;
			if ( ( ape_is_console_open() && !( state & APE_INPUT_STATE_RELEASED ) ) || ( !( state & APE_INPUT_STATE_DOWN ) && !( state & APE_INPUT_STATE_PRESSED ) ) )
			{
				continue;
			}

			//TODO: callback should take controller index
			action->callback( state, action->id );
		}
	}
	for ( unsigned int i = 0; i < action->numKeyBinds; ++i )
	{
		ApeInputState state = inputKeyboard.keys[ action->keys[ i ] ].state;
		if ( ( ape_is_console_open() && !( state & APE_INPUT_STATE_RELEASED ) ) || ( !( state & APE_INPUT_STATE_DOWN ) && !( state & APE_INPUT_STATE_PRESSED ) ) )
		{
			continue;
		}

		action->callback( state, action->id );
	}
}

static bool sdlInputInitialized = false;

/**
 * Checks for any new controllers. Would use PollEvents, but don't want
 * to fuck with the queue and also don't want *more* callbacks.
 */
static void check_for_controllers( void )
{
	int             num;
	SDL_JoystickID *joysticks = SDL_GetJoysticks( &num );
	if ( joysticks == nullptr )
	{
		return;
	}

	if ( num > CLIENT_INPUT_MAX_CONTROLLERS )
	{
		num = CLIENT_INPUT_MAX_CONTROLLERS;
	}

	for ( int i = 0; i < num; ++i )
	{
		if ( !SDL_IsGamepad( joysticks[ i ] ) )
		{
			continue;
		}

		bool isMatched = false;
		for ( unsigned int j = 0; j < CLIENT_INPUT_MAX_CONTROLLERS; ++j )
		{
			if ( inputControllers[ j ].sdlGameController == NULL )
			{
				continue;
			}

			SDL_Joystick  *joystick     = SDL_GetGamepadJoystick( inputControllers[ j ].sdlGameController );
			SDL_JoystickID compareJoyId = SDL_GetJoystickID( joystick );
			if ( compareJoyId == joysticks[ i ] )
			{
				isMatched = true;
				break;
			}
		}

		if ( isMatched )
		{// nah, not new
			continue;
		}

		// try and fetch an empty slot - break if one isn't available
		unsigned int id;
		unsigned int slot = get_empty_controller( &id );
		if ( slot == ( unsigned int ) -1 )
		{
			break;
		}

		if ( ( inputControllers[ slot ].sdlGameController = SDL_OpenGamepad( joysticks[ i ] ) ) == NULL )
		{
			ape_console_warning_( "Failed to open game controller: %s\n", SDL_GetError() );
			continue;
		}

		const char *name = SDL_GetGamepadName( inputControllers[ slot ].sdlGameController );
		if ( name == NULL )
		{
			name = "Unknown";
		}
		const char *serial = SDL_GetGamepadSerial( inputControllers[ slot ].sdlGameController );
		if ( serial == NULL )
		{
			serial = "Unknown";
		}

		inputControllers[ slot ].deadzones = qm_math_vector2f( DEFAULT_DEADZONE, DEFAULT_DEADZONE );

		if ( inputControllers[ slot ].activeButtonList == nullptr )
		{
			inputControllers[ slot ].activeButtonList = qm_os_linked_list_create();
		}
		qm_os_linked_list_clear( inputControllers[ slot ].activeButtonList );

		char tmp[ 512 ];
		snprintf( tmp, sizeof( tmp ), "Opened controller %d: %s (%s)\n", id, name, serial );
		ape_console_print_( "%s", tmp );

		numControllers++;
	}

	SDL_free( joysticks );
}

static void unregister_controller( unsigned int slot )
{
	if ( inputControllers[ slot ].sdlGameController != NULL )
	{
		SDL_CloseGamepad( inputControllers[ slot ].sdlGameController );
		inputControllers[ slot ].sdlGameController = nullptr;
	}
	PL_ZERO_( inputControllers[ slot ] );

	if ( numControllers > 0 )
	{
		numControllers--;
	}

	qm_os_memory_free( inputControllers[ slot ].activeButtonList );
	inputControllers[ slot ].activeButtonList = nullptr;

	memset( inputControllers[ slot ].buttons, 0, sizeof( Button ) );
}

static bool get_sdl_button_state( SDL_Gamepad *gameController, ApeInputButton button )
{
	SDL_GamepadButton sdlButton;
	switch ( button )
	{
		case APE_INPUT_UP:
			sdlButton = SDL_GAMEPAD_BUTTON_DPAD_UP;
			break;
		case APE_INPUT_DOWN:
			sdlButton = SDL_GAMEPAD_BUTTON_DPAD_DOWN;
			break;
		case INPUT_LEFT:
			sdlButton = SDL_GAMEPAD_BUTTON_DPAD_LEFT;
			break;
		case INPUT_RIGHT:
			sdlButton = SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
			break;

		case INPUT_LEFT_STICK:
			sdlButton = SDL_GAMEPAD_BUTTON_LEFT_STICK;
			break;
		case INPUT_RIGHT_STICK:
			sdlButton = SDL_GAMEPAD_BUTTON_RIGHT_STICK;
			break;

		case INPUT_START:
			sdlButton = SDL_GAMEPAD_BUTTON_START;
			break;
		case INPUT_BACK:
			sdlButton = SDL_GAMEPAD_BUTTON_BACK;
			break;

		case INPUT_A:
			sdlButton = SDL_GAMEPAD_BUTTON_SOUTH;
			break;
		case INPUT_B:
			sdlButton = SDL_GAMEPAD_BUTTON_EAST;
			break;
		case INPUT_X:
			sdlButton = SDL_GAMEPAD_BUTTON_WEST;
			break;
		case INPUT_Y:
			sdlButton = SDL_GAMEPAD_BUTTON_NORTH;
			break;

		case INPUT_LB:
			sdlButton = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
			break;
		case INPUT_RB:
			sdlButton = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
			break;

		default:
			return false;
	}

	return SDL_GetGamepadButton( gameController, sdlButton );
}

static float clamp_axis_input( float value, float deadzone )
{
	float t = fabsf( value ) - deadzone;
	return copysignf( ( float ) ( t > 0.0f ), value ) * fminf( t / ( 1.0f - deadzone ), 1.0f ) * ( float ) ( t > 0.0f );
}

static void list_actions_command( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	ApeInputAction *action;
	QM_OS_LINKED_LIST_ITERATE( action, actionableList, i )
	{
		ape_console_print_( "%s (%u) (%u)\n", action->id, action->numButtonBinds, action->numKeyBinds );
	}
}

static void dump_actions_command( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	static const char *FILENAME = "dump.txt";

	FILE *file = fopen( FILENAME, "w" );
	if ( file == nullptr )
	{
		ape_console_warning_( "Failed to open destination file (%s)!\n", FILENAME );
		return;
	}

	ApeInputAction *action;
	QM_OS_LINKED_LIST_ITERATE( action, actionableList, i )
	{
		fprintf( file, "%s", action->id );
	}

	fclose( file );
}

static void register_console_commands()
{
	PlRegisterConsoleCommand( "input_list_actions", "List all registered input actions.", 0, list_actions_command );
	PlRegisterConsoleCommand( "input_dump_actions", "Dumps all of the available actions to a file.", 0, dump_actions_command );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_input_initialize_( void )
{
	inputKeyboard.activeKeyList = qm_os_linked_list_create();
	if ( inputKeyboard.activeKeyList == NULL )
	{
		ape_console_error_( true, "Failed to create active key list: %s\n", PlGetError() );
	}

	// initialize the controller structure
	ape_clear_input_devices();

	if ( !SDL_Init( SDL_INIT_GAMEPAD ) )
	{
		ape_console_warning_( "Failed to initialize input: %s\n", SDL_GetError() );
		return;
	}

	// load in the game controller mappings for sdl2
	PLFile *mapFile = PlOpenFile( "mappings/gamecontrollerdb.txt", false );
	if ( mapFile != NULL )
	{
		// read it into a null-terminated buffer
		size_t size = PlGetFileSize( mapFile );
		char  *buf  = QM_OS_MEMORY_NEW_( char, size + 1 );
		PlReadFile( mapFile, buf, sizeof( char ), size );
		PlCloseFile( mapFile );

		SDL_IOStream *rw = SDL_IOFromMem( buf, ( int ) ( size + 1 ) );
		if ( SDL_AddGamepadMappingsFromIO( rw, true ) == -1 )
		{
			ape_console_warning_( "Failed to parse game controller mappings: %s\n", SDL_GetError() );
		}
	}
	else
	{
		ape_console_warning_( "Failed to load game controller mappings: %s\n", PlGetError() );
	}

	// attempt to fetch and then init config
	AcmBranch *userConfig = ape_get_user_config();
	inputConfig           = acm_get_child_by_name( userConfig, SERIALISATION_NODE_NAME );
	if ( inputConfig == NULL )
	{
		inputConfig = acm_push_object( userConfig, SERIALISATION_NODE_NAME );
	}

	check_for_controllers();

	sdlInputInitialized = true;

	ape_serialize_input_config_( inputConfig );

	register_console_commands();
}

void ape_shutdown_input_( void )
{
	ape_clear_input_devices();

	qm_os_memory_free( inputKeyboard.activeKeyList );
	inputKeyboard.activeKeyList = nullptr;
}

void ape_serialize_input_config_( AcmBranch *root )
{
#if 0
	AcmBranch *controllersBranch = acm_push_array_object( root, "controllers" );
	for ( unsigned int i = 0; i < numControllers; ++i )
	{
		AcmErrorCode errorCode;
		AcmBranch   *controllerBranch = acm_serialize_struct( &ApeInputController_descriptor, &controllers[ i ], &errorCode );
		if ( errorCode != ND_ERROR_SUCCESS )
		{
			ape_console_warning_( "Failed to serialize controller: %s\n", acm_get_error_message() );
			break;
		}

		acm_push_branch( controllersBranch, controllerBranch );
	}
#endif
}

void ape_deserialize_input_config_( AcmBranch *root )
{
	AcmBranch *inputNode = acm_get_child_by_name( root, SERIALISATION_NODE_NAME );
	if ( inputNode == NULL )
	{
		return;
	}
}

void ape_clear_input_devices( void )
{
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
	{
		unregister_controller( i );
	}
	numControllers = 0;

	// Zero out and clear the active keys
	memset( inputKeyboard.keys, 0, sizeof( Button ) );
	qm_os_linked_list_clear( inputKeyboard.activeKeyList );
}

unsigned int ape_input_register_device( SS_Acl_InputDeviceType type )
{
	unsigned int id;
	unsigned int slot = get_empty_controller( &id );
	if ( slot == ( unsigned int ) -1 )
	{
		ape_console_warning_( "Failed to find an empty input device slot!\n" );
		return -1;
	}

	PL_ZERO_( inputControllers[ slot ] );
	inputControllers[ slot ].isActive = true;

	numControllers++;

	return id;
}

bool ape_console_handle_key_event_( int key, unsigned int keyState );
void ape_client_input_handle_key_event_( int keyIndex, bool isPressed )
{
	// update the key state
	if ( keyIndex >= APE_MAX_KEY_INPUTS )
	{
		ape_console_warning_( "Received invalid key: %d\n", keyIndex );
		return;
	}

	Button *key = &inputKeyboard.keys[ keyIndex ];
	if ( isPressed && key->state == APE_INPUT_STATE_NONE )
	{
		key->state |= ( APE_INPUT_STATE_PRESSED | APE_INPUT_STATE_DOWN );
	}
	else if ( !isPressed && key->state & APE_INPUT_STATE_DOWN )
	{
		key->state &= ~APE_INPUT_STATE_DOWN | APE_INPUT_STATE_PRESSED;
		key->state |= APE_INPUT_STATE_RELEASED;
	}

	if ( key->state != APE_INPUT_STATE_NONE && key->activeNode == NULL )
	{
		key->activeNode = qm_os_linked_list_push_back( inputKeyboard.activeKeyList, key );
	}

	if ( ape_console_handle_key_event_( keyIndex, isPressed ? APE_INPUT_STATE_DOWN : APE_INPUT_STATE_NONE ) )
	{
		return;
	}
}

void ape_client_input_handle_mouse_button_event_( int button, ApeInputState buttonState )
{
	guiUpdateMouseButton( button, ( buttonState == APE_INPUT_STATE_DOWN ) );

	if ( buttonState != APE_INPUT_STATE_RELEASED && ( inputMouse.buttons[ button ] == APE_INPUT_STATE_PRESSED || inputMouse.buttons[ button ] == APE_INPUT_STATE_DOWN ) )
	{
		return;
	}

	inputMouse.buttons[ button ] = buttonState;
}

bool ape_console_handle_mouse_wheel_event_( float x, float y );
void ape_client_input_handle_mouse_wheel_event( float x, float y )
{
	inputMouse.oldWheel = inputMouse.wheel;
	inputMouse.wheel.x  = x;
	inputMouse.wheel.y  = y;

	if ( ape_console_handle_mouse_wheel_event_( x, y ) )
	{
		return;
	}

	gui_update_mouse_wheel( x, y );
}

void Client_Input_HandleMouseMotionEvent( int x, int y )
{
	inputMouse.ox = inputMouse.x;
	inputMouse.oy = inputMouse.y;
	inputMouse.x  = x;
	inputMouse.y  = y;

	ape_gui_update_mouse_position_( inputMouse.x, inputMouse.y );
}

void ape_client_input_get_mouse_position( int *x, int *y )
{
	*x = inputMouse.x;
	*y = inputMouse.y;
}

void ape_client_input_get_mouse_delta( int *x, int *y )
{
	*x = inputMouse.dx;
	*y = inputMouse.dy;
}

void ape_begin_input_frame_( void )
{
	// Ensure we store the old x/y
	//int ox = inputMouse.x;
	//int oy = inputMouse.y;

	// Calculate delta
	if ( !ape_is_console_open() )
	{
		int w, h;
		shell_get_window_size( &w, &h );

		inputMouse.dx = ( ( w / 2 ) - inputMouse.x );
		inputMouse.dy = ( ( h / 2 ) - inputMouse.y );
	}
}

void ape_input_tick_( void )
{
	if ( !sdlInputInitialized )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	if ( actionableList != NULL )
	{
		const ApeInputAction *action;
		QM_OS_LINKED_LIST_ITERATE( action, actionableList, i )
		{
			iterate_action( action );
		}
	}

	// now update the state of all the connected devices
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
	{
		if ( inputControllers[ i ].sdlGameController == NULL )
		{
			continue;
		}

		if ( !SDL_GamepadConnected( inputControllers[ i ].sdlGameController ) )
		{
			ape_console_print_( "Controller disconnected from slot %u.\n", i );
			unregister_controller( i );
			continue;
		}

		Button *button;
		// first update the state for any controllers from the last tick
		QM_OS_LINKED_LIST_ITERATE( button, inputControllers[ i ].activeButtonList, j )
		{
			button->state &= ~( APE_INPUT_STATE_PRESSED | APE_INPUT_STATE_RELEASED );
			qm_os_memory_free( button->activeNode );
			button->activeNode = nullptr;
		}

		for ( unsigned int j = 0; j < APE_MAX_BUTTON_INPUTS; ++j )
		{
			button = &inputControllers[ i ].buttons[ j ];

			bool state = get_sdl_button_state( inputControllers[ i ].sdlGameController, j );
			if ( state && button->state == APE_INPUT_STATE_NONE )
			{
				button->state |= APE_INPUT_STATE_PRESSED | APE_INPUT_STATE_DOWN;
			}
			else if ( !state && button->state & APE_INPUT_STATE_DOWN )
			{
				button->state &= ~APE_INPUT_STATE_DOWN | APE_INPUT_STATE_PRESSED;
				button->state |= APE_INPUT_STATE_RELEASED;
			}

			if ( button->state != APE_INPUT_STATE_NONE && button->activeNode == NULL )
			{
				button->activeNode = qm_os_linked_list_push_back( inputControllers[ i ].activeButtonList, button );
			}
		}

		inputControllers[ i ].stickLOld = inputControllers[ i ].stickL;
		inputControllers[ i ].stickROld = inputControllers[ i ].stickR;

		inputControllers[ i ].stickL.x = clamp_axis_input( ( ( float ) SDL_GetGamepadAxis( inputControllers[ i ].sdlGameController, SDL_GAMEPAD_AXIS_LEFTX ) ) / ( float ) INT16_MAX, inputControllers[ i ].deadzones.x );
		inputControllers[ i ].stickL.y = clamp_axis_input( ( ( float ) SDL_GetGamepadAxis( inputControllers[ i ].sdlGameController, SDL_GAMEPAD_AXIS_LEFTY ) ) / ( float ) INT16_MAX, inputControllers[ i ].deadzones.x );

		inputControllers[ i ].stickR.x = clamp_axis_input( ( ( float ) SDL_GetGamepadAxis( inputControllers[ i ].sdlGameController, SDL_GAMEPAD_AXIS_RIGHTX ) ) / ( float ) INT16_MAX, inputControllers[ i ].deadzones.y );
		inputControllers[ i ].stickR.y = clamp_axis_input( ( ( float ) SDL_GetGamepadAxis( inputControllers[ i ].sdlGameController, SDL_GAMEPAD_AXIS_RIGHTY ) ) / ( float ) INT16_MAX, inputControllers[ i ].deadzones.y );

		inputControllers[ i ].stickLDelta = qm_math_vector2f_sub( inputControllers[ i ].stickLOld, inputControllers[ i ].stickL );
		inputControllers[ i ].stickRDelta = qm_math_vector2f_sub( inputControllers[ i ].stickROld, inputControllers[ i ].stickR );
	}

	Button *key;
	QM_OS_LINKED_LIST_ITERATE( key, inputKeyboard.activeKeyList, i )
	{
		key->state &= ~( APE_INPUT_STATE_PRESSED | APE_INPUT_STATE_RELEASED );
		qm_os_memory_free( key->activeNode );
		key->activeNode = nullptr;
	}

	// poll for new devices
	// FYI: rewrote this so that it doesn't interrupt keyboard input used for console etc., as PollEvent *will* unfortunately
	check_for_controllers();

	COM_PROFILE_FUNCTION_END();
}

void ape_input_center_mouse( void )
{
	int w, h;
	shell_get_window_size( &w, &h );

	if ( !ape_is_console_open() )
	{
		shell_set_mouse_position( w / 2, h / 2 );
	}

	inputMouse.x = ( w / 2 );
	inputMouse.y = ( h / 2 );
}

void ape_end_input_frame_( void )
{
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook == NULL || !mouseLook->b_value )
	{
		return;
	}

	ape_input_center_mouse();
}

unsigned int apeGetNumControllers( void ) { return numControllers; }

ApeInputState ape_client_input_get_button_state( unsigned int slot, ApeInputButton button )
{
	assert( slot < CLIENT_INPUT_MAX_CONTROLLERS );
	return inputControllers[ slot ].buttons[ button ].state;
}

QmMathVector2f ape_client_input_get_controller_axis_state( unsigned int slot, unsigned int stickNum )
{
	assert( slot < CLIENT_INPUT_MAX_CONTROLLERS );
	return ( stickNum == 0 ) ? inputControllers[ slot ].stickL : inputControllers[ slot ].stickR;
}

void ape_client_input_register_action( const char    *id,
                                       ApeInputButton buttons[], unsigned int numDefaultButtons,
                                       ApeInputKey keys[], unsigned int numDefaultKeys,
                                       ApeInputActionCallback actionCallback )
{
	/* if the list has not been allocated yet, do the deed */
	if ( actionableList == NULL )
	{
		actionableList = qm_os_linked_list_create();
		if ( actionableList == NULL )
		{
			ape_console_error_( true, "Failed to create actionable list: %s\n", PlGetError() );
		}
	}

	if ( numDefaultButtons > APE_MAX_BUTTON_INPUTS )
	{
		numDefaultButtons = APE_MAX_BUTTON_INPUTS;
		ape_console_warning_( "Too many default button inputs for action!\n" );
	}
	if ( numDefaultKeys > APE_MAX_KEY_INPUTS )
	{
		numDefaultKeys = APE_MAX_KEY_INPUTS;
		ape_console_warning_( "Too many default key inputs for action!\n" );
	}

	ApeInputAction *inputAction = QM_OS_MEMORY_NEW( ApeInputAction );
	if ( inputAction == nullptr )
	{
		ape_console_warning_( "Failed to allocate action (%s)!\n", id );
		return;
	}

	snprintf( inputAction->id, sizeof( inputAction->id ), "%s", id );
	inputAction->callback = actionCallback;

	memcpy( inputAction->buttons, buttons, sizeof( ApeInputButton ) * numDefaultButtons );
	inputAction->numButtonBinds = numDefaultButtons;

	memcpy( inputAction->keys, keys, sizeof( ApeInputKey ) * numDefaultKeys );
	inputAction->numKeyBinds = numDefaultKeys;

	inputAction->node = qm_os_linked_list_push_back( actionableList, inputAction );
}
