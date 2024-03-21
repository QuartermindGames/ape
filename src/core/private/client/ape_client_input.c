// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Client input management.
// Author:  Mark E. Sowden

#include <SDL2/SDL.h>

#include "ape_private.h"
#include "ape_client_input.h"
#include "gui/gui_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static NdBranch *inputConfig;

static const float DEFAULT_DEADZONE = 0.2f;

#define SERIALISATION_NODE_NAME "input"

typedef struct ApeInputAction
{
	char id[ 32 ];
	ApeInputActionCallback callback;

	ApeInputButton buttons[ APE_MAX_BUTTON_INPUTS ];
	unsigned int numButtonBinds;

	ApeInputKey keys[ APE_MAX_KEY_INPUTS ];
	unsigned int numKeyBinds;

	PLLinkedListNode *node;
} ApeInputAction;

static PLLinkedList *actionableList = NULL;

static struct
{
	int x, y;
	int ox, oy;
	int dx, dy;
	ApeInputState buttons[ APE_MAX_INPUT_MOUSE_BUTTONS ];

	PLVector2 wheel, oldWheel;
} inputMouse = {};

typedef struct Key
{
	ApeInputState state;
	PLLinkedListNode *activeNode;
} Key;

static struct
{
	Key keys[ APE_MAX_KEY_INPUTS ];
	PLLinkedList *activeKeyList;
} inputKeyboard = {};

typedef struct ApeInputController
{
	bool isActive;

	ApeInputState buttons[ APE_MAX_BUTTON_INPUTS ];

	PLVector2 stickL, stickLOld, stickLDelta;
	PLVector2 stickR, stickROld, stickRDelta;
	PLVector2 deadzones;

	SDL_GameController *sdlGameController;
} ApeInputController;

ND_DECLARE_STRUCT( ApeInputController, 2,
                   ND_DECLARE_STRUCT_ITEM_ARRAY( ApeInputController, deadzones, ND_PROPERTY_FLOAT32, 2 ) )

#define CLIENT_INPUT_MAX_CONTROLLERS 4
static ApeInputController controllers[ CLIENT_INPUT_MAX_CONTROLLERS ];
static unsigned int numControllers = 0;

static ApeInputController *get_empty_controller( unsigned int *id )
{
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
	{
		if ( controllers[ i ].isActive )
		{
			continue;
		}

		*id = i;
		return &controllers[ i ];
	}

	return NULL;
}

static void iterate_action( void *userData, PL_UNUSED bool *breakEarly )
{
	ApeInputAction *action = ( ApeInputAction * ) userData;
	for ( unsigned int i = 0; i < action->numButtonBinds; ++i )
	{
		ApeInputState state = ss_shell_get_button_state( action->buttons[ i ] );
		if ( ( ape_is_console_open() && state != APE_INPUT_STATE_RELEASED ) || ( ( state != APE_INPUT_STATE_DOWN ) && ( state != APE_INPUT_STATE_PRESSED ) ) )
		{
			continue;
		}

		action->callback( state, action->id );
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
	int num = SDL_NumJoysticks();
	if ( num < 0 )
	{
		PRINT_WARNING( "Failed to fetch the number of available joysticks: %s\n", SDL_GetError() );
		return;
	}
	else if ( num > CLIENT_INPUT_MAX_CONTROLLERS )
	{
		num = CLIENT_INPUT_MAX_CONTROLLERS;
	}

	for ( int i = 0; i < num; ++i )
	{
		if ( !SDL_IsGameController( i ) )
		{
			continue;
		}

		// right, uh, check if it's already open
		SDL_JoystickID joyId = SDL_JoystickGetDeviceInstanceID( i );
		bool isMatched = false;
		for ( unsigned int j = 0; j < CLIENT_INPUT_MAX_CONTROLLERS; ++j )
		{
			if ( controllers[ j ].sdlGameController == NULL )
			{
				continue;
			}

			SDL_JoystickID compareJoyId = SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( controllers[ j ].sdlGameController ) );
			if ( compareJoyId == joyId )
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
		ApeInputController *controller = get_empty_controller( &id );
		if ( controller == NULL )
		{
			break;
		}

		if ( ( controller->sdlGameController = SDL_GameControllerOpen( i ) ) == NULL )
		{
			PRINT_WARNING( "Failed to open game controller: %s\n", SDL_GetError() );
			continue;
		}

		const char *name = SDL_GameControllerName( controller->sdlGameController );
		if ( name == NULL )
		{
			name = "Unknown";
		}
		const char *serial = SDL_GameControllerGetSerial( controller->sdlGameController );
		if ( serial == NULL )
		{
			serial = "Unknown";
		}

		controller->deadzones = PL_VECTOR2( DEFAULT_DEADZONE, DEFAULT_DEADZONE );

		char tmp[ 512 ];
		snprintf( tmp, sizeof( tmp ), "Opened controller %d: %s (%s)\n", id, name, serial );
		PRINT( "%s", tmp );

		numControllers++;
	}
}

static void unregister_controller( unsigned int id )
{
	if ( controllers[ id ].sdlGameController != NULL )
	{
		SDL_GameControllerClose( controllers[ id ].sdlGameController );
		controllers[ id ].sdlGameController = NULL;
	}
	PL_ZERO_( controllers[ id ] );

	if ( numControllers > 0 )
	{
		numControllers--;
	}
}

static bool get_sdl_button_state( SDL_GameController *gameController, ApeInputButton button )
{
	SDL_GameControllerButton sdlButton;
	switch ( button )
	{
		case APE_INPUT_UP:
			sdlButton = SDL_CONTROLLER_BUTTON_DPAD_UP;
			break;
		case APE_INPUT_DOWN:
			sdlButton = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
			break;
		case INPUT_LEFT:
			sdlButton = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
			break;
		case INPUT_RIGHT:
			sdlButton = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
			break;

		case INPUT_LEFT_STICK:
			sdlButton = SDL_CONTROLLER_BUTTON_LEFTSTICK;
			break;
		case INPUT_RIGHT_STICK:
			sdlButton = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
			break;

		case INPUT_START:
			sdlButton = SDL_CONTROLLER_BUTTON_START;
			break;
		case INPUT_BACK:
			sdlButton = SDL_CONTROLLER_BUTTON_BACK;
			break;

		case INPUT_A:
			sdlButton = SDL_CONTROLLER_BUTTON_A;
			break;
		case INPUT_B:
			sdlButton = SDL_CONTROLLER_BUTTON_B;
			break;
		case INPUT_X:
			sdlButton = SDL_CONTROLLER_BUTTON_X;
			break;
		case INPUT_Y:
			sdlButton = SDL_CONTROLLER_BUTTON_Y;
			break;

		case INPUT_LB:
			sdlButton = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
			break;
		case INPUT_RB:
			sdlButton = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
			break;

		default:
			return false;
	}

	return SDL_GameControllerGetButton( gameController, sdlButton );
}

static float clamp_axis_input( float value, float deadzone )
{
	float t = fabsf( value ) - deadzone;
	return copysignf( ( float ) ( t > 0.0f ), value ) * fminf( t / ( 1.0f - deadzone ), 1.0f ) * ( float ) ( t > 0.0f );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_input_( void )
{
	inputKeyboard.activeKeyList = PlCreateLinkedList();
	if ( inputKeyboard.activeKeyList == NULL )
	{
		PRINT_ERROR( "Failed to create active key list: %s\n", PlGetError() );
	}

	// initialize the controller structure
	ape_clear_input_devices();

	if ( SDL_Init( SDL_INIT_GAMECONTROLLER ) != 0 )
	{
		PRINT_WARNING( "Failed to initialize SDL input: %s\n", SDL_GetError() );
		return;
	}

	// load in the game controller mappings for sdl2
	PLFile *mapFile = PlOpenFile( "mappings/gamecontrollerdb.txt", false );
	if ( mapFile != NULL )
	{
		// read it into a null-terminated buffer
		size_t size = PlGetFileSize( mapFile );
		char *buf = PL_NEW_( char, size + 1 );
		PlReadFile( mapFile, buf, sizeof( char ), size );
		PlCloseFile( mapFile );

		SDL_RWops *rw = SDL_RWFromMem( buf, ( int ) ( size + 1 ) );
		if ( SDL_GameControllerAddMappingsFromRW( rw, true ) == -1 )
		{
			PRINT_WARNING( "Failed to parse game controller mappings: %s\n", SDL_GetError() );
		}
	}
	else
	{
		PRINT_WARNING( "Failed to load game controller mappings: %s\n", PlGetError() );
	}

	// attempt to fetch and then init config
	NdBranch *userConfig = ape_get_user_config();
	inputConfig = nd_branch_get_child_by_name( userConfig, SERIALISATION_NODE_NAME );
	if ( inputConfig == NULL )
	{
		inputConfig = nd_branch_push_back_object( userConfig, SERIALISATION_NODE_NAME );
	}

	check_for_controllers();

	sdlInputInitialized = true;

	ape_serialize_input_config_( NULL );
}

void ape_shutdown_input_( void )
{
	ape_clear_input_devices();

	PlDestroyLinkedList( inputKeyboard.activeKeyList );
	inputKeyboard.activeKeyList = NULL;
}

void ape_serialize_input_config_( NdBranch *root )
{
	NdBranch *controllersBranch = nd_branch_push_back_object_array( root, "controllers" );
	for ( unsigned int i = 0; i < numControllers; ++i )
	{
		NdErrorCode errorCode;
		NdBranch *controllerBranch = nd_serialize_struct( &ApeInputController_descriptor, &controllers[ i ], &errorCode );
		if ( errorCode != ND_ERROR_SUCCESS )
		{
			ape_warning_( "Failed to serialize controller: %s\n", nd_get_error_message() );
			break;
		}

		nd_branch_push_back_branch( controllersBranch, controllerBranch );
	}

	nd_write_file( "test.cfg.n", controllersBranch, ND_FILE_UTF8 );
}

void ape_deserialize_input_config_( NdBranch *root )
{
	NdBranch *inputNode = nd_branch_get_child_by_name( root, SERIALISATION_NODE_NAME );
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
	memset( inputKeyboard.keys, 0, sizeof( Key ) );
	PlDestroyLinkedListNodes( inputKeyboard.activeKeyList );
}

unsigned int ape_input_register_device( SS_Acl_InputDeviceType type )
{
	unsigned int id;
	ApeInputController *device = get_empty_controller( &id );
	if ( device == NULL )
	{
		PRINT_WARNING( "Failed to find an empty input device slot!\n" );
		return ( unsigned int ) -1;
	}

	PL_ZERO( device, sizeof( ApeInputController ) );

	device->isActive = true;

	numControllers++;

	return id;
}

bool ape_console_handle_key_event_( int key, unsigned int keyState );
void ape_client_input_handle_key_event_( int keyIndex, bool isPressed )
{
	// update the key state
	if ( keyIndex >= APE_MAX_KEY_INPUTS )
	{
		PRINT_WARNING( "Received invalid key: %d\n", keyIndex );
		return;
	}

	Key *key = &inputKeyboard.keys[ keyIndex ];
	if ( isPressed && key->state == APE_INPUT_STATE_NONE )
	{
		key->state |= ( APE_INPUT_STATE_PRESSED | APE_INPUT_STATE_DOWN );
	}
	else if ( key->state & APE_INPUT_STATE_DOWN )
	{
		key->state &= ~APE_INPUT_STATE_DOWN | APE_INPUT_STATE_PRESSED;
		key->state |= APE_INPUT_STATE_RELEASED;
	}

	if ( key->state != APE_INPUT_STATE_NONE && key->activeNode == NULL )
	{
		key->activeNode = PlInsertLinkedListNode( inputKeyboard.activeKeyList, key );
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
	inputMouse.wheel.x = x;
	inputMouse.wheel.y = y;

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
	inputMouse.x = x;
	inputMouse.y = y;

	guiUpdateMousePosition( inputMouse.x, inputMouse.y );
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

void ape_tick_input_( void )
{
	if ( !sdlInputInitialized )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	if ( actionableList != NULL )
	{
		PlIterateLinkedList( actionableList, iterate_action, true );
	}

	// now update the state of all the connected devices
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
	{
		if ( controllers[ i ].sdlGameController == NULL )
		{
			continue;
		}

		if ( !SDL_GameControllerGetAttached( controllers[ i ].sdlGameController ) )
		{
			PRINT( "Controller disconnected from slot %u.\n", i );
			unregister_controller( i );
			continue;
		}

		for ( unsigned int j = 0; j < APE_MAX_BUTTON_INPUTS; ++j )
		{
			bool state = get_sdl_button_state( controllers[ i ].sdlGameController, j );
			if ( !state )
			{
				controllers[ i ].buttons[ j ] = APE_INPUT_STATE_NONE;
				continue;
			}

			if ( controllers[ i ].buttons[ j ] == APE_INPUT_STATE_DOWN )
			{
				continue;
			}

			controllers[ i ].buttons[ j ] = ( controllers[ i ].buttons[ j ] == APE_INPUT_STATE_PRESSED ) ? APE_INPUT_STATE_DOWN : APE_INPUT_STATE_PRESSED;
		}

		controllers[ i ].stickLOld = controllers[ i ].stickL;
		controllers[ i ].stickROld = controllers[ i ].stickR;

		controllers[ i ].stickL.x = clamp_axis_input( ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_LEFTX ) ) / ( float ) INT16_MAX, controllers[ i ].deadzones.x );
		controllers[ i ].stickL.y = clamp_axis_input( ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_LEFTY ) ) / ( float ) INT16_MAX, controllers[ i ].deadzones.x );

		controllers[ i ].stickR.x = clamp_axis_input( ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_RIGHTX ) ) / ( float ) INT16_MAX, controllers[ i ].deadzones.y );
		controllers[ i ].stickR.y = clamp_axis_input( ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_RIGHTY ) ) / ( float ) INT16_MAX, controllers[ i ].deadzones.y );

		controllers[ i ].stickLDelta = PlSubtractVector2( &controllers[ i ].stickLOld, &controllers[ i ].stickL );
		controllers[ i ].stickRDelta = PlSubtractVector2( &controllers[ i ].stickROld, &controllers[ i ].stickR );
	}

	PLLinkedListNode *node = PlGetFirstNode( inputKeyboard.activeKeyList );
	while ( node != NULL )
	{
		Key *key = PlGetLinkedListNodeUserData( node );
		node = PlGetNextLinkedListNode( node );
		key->state &= ~( APE_INPUT_STATE_PRESSED | APE_INPUT_STATE_RELEASED );
		PlDestroyLinkedListNode( key->activeNode );
		key->activeNode = NULL;
	}

	//PRINT( "L: %s\n", PlPrintVector2( &controllers[ 0 ].stickL, PL_VAR_F32 ) );
	//PRINT( "R: %s\n", PlPrintVector2( &controllers[ 0 ].stickR, PL_VAR_F32 ) );

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
	if ( slot >= CLIENT_INPUT_MAX_CONTROLLERS )
	{
		return APE_INPUT_STATE_NONE;
	}

	return controllers[ slot ].buttons[ button ];
}

PLVector2 ape_client_input_get_controller_axis_state( unsigned int slot, unsigned int stickNum )
{
	assert( slot < CLIENT_INPUT_MAX_CONTROLLERS );
	if ( slot >= CLIENT_INPUT_MAX_CONTROLLERS )
	{
		return pl_vecOrigin2;
	}

	return ( stickNum == 0 ) ? controllers[ slot ].stickL : controllers[ slot ].stickR;
}

void ape_client_input_register_action( const char *id,
                                       ApeInputButton buttons[], unsigned int numDefaultButtons,
                                       ApeInputKey keys[], unsigned int numDefaultKeys,
                                       ApeInputActionCallback actionCallback )
{
	/* if the list has not been allocated yet, do the deed */
	if ( actionableList == NULL )
	{
		actionableList = PlCreateLinkedList();
		if ( actionableList == NULL )
		{
			PRINT_ERROR( "Failed to create actionable list: %s\n", PlGetError() );
		}
	}

	if ( numDefaultButtons > APE_MAX_BUTTON_INPUTS )
	{
		numDefaultButtons = APE_MAX_BUTTON_INPUTS;
		PRINT_WARNING( "Too many default button inputs for action!\n" );
	}
	if ( numDefaultKeys > APE_MAX_KEY_INPUTS )
	{
		numDefaultKeys = APE_MAX_KEY_INPUTS;
		PRINT_WARNING( "Too many default key inputs for action!\n" );
	}

	ApeInputAction *inputAction = PL_NEW( ApeInputAction );
	snprintf( inputAction->id, sizeof( inputAction->id ), "%s", id );
	inputAction->callback = actionCallback;

	memcpy( inputAction->buttons, buttons, sizeof( ApeInputButton ) * numDefaultButtons );
	inputAction->numButtonBinds = numDefaultButtons;

	memcpy( inputAction->keys, keys, sizeof( ApeInputKey ) * numDefaultKeys );
	inputAction->numKeyBinds = numDefaultKeys;

	inputAction->node = PlInsertLinkedListNode( actionableList, inputAction );
}
