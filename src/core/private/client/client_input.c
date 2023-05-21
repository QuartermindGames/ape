// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <SDL2/SDL.h>

#include "core_private.h"
#include "client_input.h"
#include "gui/gui_private.h"

#include <yin/node.h>

/****************************************
 * PRIVATE
 ****************************************/

#define SERIALISATION_NODE_NAME "input"

typedef struct ClientInputAction
{
	char id[ 32 ];
	ClientInputActionCallback callback;

	YNCoreInputButton buttons[ YN_CORE_MAX_BUTTON_INPUTS ];
	unsigned int numButtonBinds;

	YNCoreInputKey keys[ YN_CORE_MAX_KEY_INPUTS ];
	unsigned int numKeyBinds;

	PLLinkedListNode *node;
} ClientInputAction;

static PLLinkedList *actionableList = NULL;

static struct
{
	int x, y;
	int ox, oy;
	int dx, dy;
	YNCoreInputState buttons[ YN_CORE_MAX_INPUT_MOUSE_BUTTONS ];

	PLVector2 wheel, oldWheel;
} inputMouse;

static struct
{
	YNCoreInputState keys[ YN_CORE_MAX_KEY_INPUTS ];
} inputKeyboard;

typedef struct ClientInputController
{
	bool isActive;
	YNCoreInputState buttons[ YN_CORE_MAX_BUTTON_INPUTS ];
	PLVector2 stickL, stickLOld, stickLDelta;
	PLVector2 stickR, stickROld, stickRDelta;
	SDL_GameController *sdlGameController;
} ClientInputController;

#define CLIENT_INPUT_MAX_CONTROLLERS 4
static ClientInputController controllers[ CLIENT_INPUT_MAX_CONTROLLERS ];
static unsigned int numControllers = 0;

static ClientInputController *GetEmptyController( unsigned int *id )
{
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
	{
		if ( controllers[ i ].isActive )
			continue;

		*id = i;
		return &controllers[ i ];
	}

	return NULL;
}

static void IterateAction( void *userData, PL_UNUSED bool *breakEarly )
{
	ClientInputAction *action = ( ClientInputAction * ) userData;
	for ( unsigned int i = 0; i < action->numButtonBinds; ++i )
	{
		YNCoreInputState state = YnCore_ShellInterface_GetButtonState( action->buttons[ i ] );
		if ( ( state != YN_CORE_INPUT_STATE_DOWN ) && ( state != YN_CORE_INPUT_STATE_PRESSED ) )
		{
			continue;
		}

		action->callback( state );
	}
	for ( unsigned int i = 0; i < action->numKeyBinds; ++i )
	{
		YNCoreInputState state = YnCore_ShellInterface_GetKeyState( action->keys[ i ] );
		if ( ( state != YN_CORE_INPUT_STATE_DOWN ) && ( state != YN_CORE_INPUT_STATE_PRESSED ) )
		{
			continue;
		}

		action->callback( state );
	}
}

/****************************************
 * PUBLIC
 ****************************************/

static bool sdlInputInitialized = false;

/**
 * Checks for any new controllers. Would use PollEvents, but don't want
 * to fuck with the queue and also don't want *more* callbacks.
 */
static void CheckForControllers( void )
{
	int num = SDL_NumJoysticks();
	if ( num < 0 )
	{
		PRINT_WARNING( "Failed to fetch the number of available joysticks: %s\n", SDL_GetError() );
		return;
	}
	else if ( num > CLIENT_INPUT_MAX_CONTROLLERS )
		num = CLIENT_INPUT_MAX_CONTROLLERS;

	for ( int i = 0; i < num; ++i )
	{
		if ( !SDL_IsGameController( i ) )
			continue;

		// right, uh, check if it's already open
		SDL_JoystickID joyId = SDL_JoystickGetDeviceInstanceID( i );
		bool isMatched       = false;
		for ( unsigned int j = 0; j < CLIENT_INPUT_MAX_CONTROLLERS; ++j )
		{
			if ( controllers[ j ].sdlGameController == NULL )
				continue;

			SDL_JoystickID compareJoyId = SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( controllers[ j ].sdlGameController ) );
			if ( compareJoyId == joyId )
			{
				isMatched = true;
				break;
			}
		}

		if ( isMatched )
			// nah, not new
			continue;

		// try and fetch an empty slot - break if one isn't available
		unsigned int id;
		ClientInputController *controller = GetEmptyController( &id );
		if ( controller == NULL )
			break;

		if ( ( controller->sdlGameController = SDL_GameControllerOpen( i ) ) == NULL )
		{
			PRINT_WARNING( "Failed to open game controller: %s\n", SDL_GetError() );
			continue;
		}

		const char *name = SDL_GameControllerName( controller->sdlGameController );
		if ( name == NULL )
			name = "Unknown";
		const char *serial = SDL_GameControllerGetSerial( controller->sdlGameController );
		if ( serial == NULL )
			serial = "Unknown";

		char tmp[ 512 ];
		snprintf( tmp, sizeof( tmp ), "Opened controller %d: %s (%s)\n", id, name, serial );
		PRINT( "%s", tmp );

		numControllers++;
	}
}

void Client_Input_Initialize( void )
{
	// initialize the controller structure
	Client_Input_ClearDevices();

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
		char *buf   = PL_NEW_( char, size + 1 );
		PlReadFile( mapFile, buf, sizeof( char ), size );
		PlCloseFile( mapFile );

		SDL_RWops *rw = SDL_RWFromMem( buf, ( int ) ( size + 1 ) );
		if ( SDL_GameControllerAddMappingsFromRW( rw, true ) == -1 )
			PRINT_WARNING( "Failed to parse game controller mappings: %s\n", SDL_GetError() );
	}
	else
		PRINT_WARNING( "Failed to load game controller mappings: %s\n", PlGetError() );

	CheckForControllers();

	sdlInputInitialized = true;
}

void Client_Input_Shutdown( void )
{
	Client_Input_ClearDevices();
}

void Client_Input_SerializeConfig( NdBranch *root )
{
	/* nothing to serialise */
	if ( actionableList == NULL )
		return;

	NdBranch *inputNode = ndPushBackObjectArray( root, SERIALISATION_NODE_NAME );
	if ( inputNode == NULL )
	{
		PRINT_WARNING( "Failed to attach \"" SERIALISATION_NODE_NAME "\" for config!\n" );
		return;
	}

	//PlIterateLinkedList( actionableList, NULL, NULL );
}

void Client_Input_DeserializeConfig( NdBranch *root )
{
	NdBranch *inputNode = ndGetChildByName( root, SERIALISATION_NODE_NAME );
	if ( inputNode == NULL )
		return;
}

static void UnregisterController( unsigned int id )
{
	if ( controllers[ id ].sdlGameController != NULL )
	{
		SDL_GameControllerClose( controllers[ id ].sdlGameController );
		controllers[ id ].sdlGameController = NULL;
	}
	PL_ZERO_( controllers[ id ] );
	numControllers--;
}

void Client_Input_ClearDevices( void )
{
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
		UnregisterController( i );
}

unsigned int Client_Input_RegisterController( ClientInputDeviceType type )
{
	unsigned int id;
	ClientInputController *device = GetEmptyController( &id );
	if ( device == NULL )
	{
		PRINT_WARNING( "Failed to find an empty input device slot!\n" );
		return ( unsigned int ) -1;
	}

	PL_ZERO( device, sizeof( ClientInputController ) );

	device->isActive = true;

	numControllers++;

	return id;
}

bool Client_Console_HandleKeyboardEvent( int key, unsigned int keyState );
void Client_Input_HandleKeyboardEvent( int key, YNCoreInputState keyState )
{
	if ( Client_Console_HandleKeyboardEvent( key, keyState ) )
		return;
}

void Client_Input_HandleMouseButtonEvent( int button, YNCoreInputState buttonState )
{
	GUI_UpdateMouseButton( button, ( buttonState == YN_CORE_INPUT_STATE_DOWN ) );

	if ( buttonState != YN_CORE_INPUT_STATE_RELEASED && ( inputMouse.buttons[ button ] == YN_CORE_INPUT_STATE_PRESSED || inputMouse.buttons[ button ] == YN_CORE_INPUT_STATE_DOWN ) )
		return;

	inputMouse.buttons[ button ] = buttonState;
}

bool Client_Console_HandleMouseWheelEvent( float x, float y );
void Client_Input_HandleMouseWheelEvent( float x, float y )
{
	inputMouse.oldWheel = inputMouse.wheel;
	inputMouse.wheel.x  = x;
	inputMouse.wheel.y  = y;

	if ( Client_Console_HandleMouseWheelEvent( x, y ) )
		return;

	GUI_UpdateMouseWheel( x, y );
}

void Client_Input_HandleMouseMotionEvent( int x, int y )
{
	inputMouse.ox = inputMouse.x;
	inputMouse.oy = inputMouse.y;
	inputMouse.x  = x;
	inputMouse.y  = y;

	GUI_UpdateMousePosition( inputMouse.x, inputMouse.y );
}

void Client_Input_GetMousePosition( int *x, int *y )
{
	*x = inputMouse.x;
	*y = inputMouse.y;
}

void Client_Input_GetMouseDelta( int *x, int *y )
{
	*x = inputMouse.dx;
	*y = inputMouse.dy;
}

void Client_Input_BeginFrame( void )
{
	// Ensure we store the old x/y
	//int ox = inputMouse.x;
	//int oy = inputMouse.y;

	int w, h;
	ogeShellInterface_GetWindowSize( &w, &h );
	int cx = w / 2;
	int cy = h / 2;

	// Calculate delta
	inputMouse.dx = ( cx - inputMouse.x );
	inputMouse.dy = ( cy - inputMouse.y );
}

static bool GetSDLButtonState( SDL_GameController *gameController, YNCoreInputButton button )
{
	SDL_GameControllerButton sdlButton;
	switch ( button )
	{
		case YN_CORE_INPUT_UP:
			sdlButton = SDL_CONTROLLER_BUTTON_DPAD_UP;
			break;
		case YN_CORE_INPUT_DOWN:
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

void Client_Input_Tick( void )
{
	if ( !sdlInputInitialized )
		return;

	// now update the state of all the connected devices
	for ( unsigned int i = 0; i < CLIENT_INPUT_MAX_CONTROLLERS; ++i )
	{
		if ( controllers[ i ].sdlGameController == NULL )
			continue;

		if ( !SDL_GameControllerGetAttached( controllers[ i ].sdlGameController ) )
		{
			PRINT( "Controller disconnected from slot %u.\n", i );
			UnregisterController( i );
			continue;
		}

		for ( unsigned int j = 0; j < YN_CORE_MAX_BUTTON_INPUTS; ++j )
		{
			bool state = GetSDLButtonState( controllers[ i ].sdlGameController, j );
			if ( !state )
			{
				controllers[ i ].buttons[ j ] = YN_CORE_INPUT_STATE_NONE;
				continue;
			}

			if ( controllers[ i ].buttons[ j ] == YN_CORE_INPUT_STATE_DOWN )
				continue;

			controllers[ i ].buttons[ j ] = ( controllers[ i ].buttons[ j ] == YN_CORE_INPUT_STATE_PRESSED ) ? YN_CORE_INPUT_STATE_DOWN : YN_CORE_INPUT_STATE_PRESSED;
		}

		controllers[ i ].stickLOld = controllers[ i ].stickL;
		controllers[ i ].stickROld = controllers[ i ].stickR;

		controllers[ i ].stickL.x = ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_LEFTX ) ) / ( float ) INT16_MAX;
		controllers[ i ].stickL.y = ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_LEFTY ) ) / ( float ) INT16_MAX;
		controllers[ i ].stickR.x = ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_RIGHTX ) ) / ( float ) INT16_MAX;
		controllers[ i ].stickR.y = ( ( float ) SDL_GameControllerGetAxis( controllers[ i ].sdlGameController, SDL_CONTROLLER_AXIS_RIGHTY ) ) / ( float ) INT16_MAX;

		controllers[ i ].stickLDelta = PlSubtractVector2( &controllers[ i ].stickLOld, &controllers[ i ].stickL );
		controllers[ i ].stickRDelta = PlSubtractVector2( &controllers[ i ].stickROld, &controllers[ i ].stickR );
	}

	//PRINT( "L: %s\n", PlPrintVector2( &controllers[ 0 ].stickL, PL_VAR_F32 ) );
	//PRINT( "R: %s\n", PlPrintVector2( &controllers[ 0 ].stickR, PL_VAR_F32 ) );

	if ( actionableList != NULL )
	{
		PlIterateLinkedList( actionableList, IterateAction, true );
	}

	// poll for new devices
	// FYI: rewrote this so that it doesn't interrupt keyboard input used for console etc., as PollEvent *will* unfortunately
	CheckForControllers();
}

void Client_Input_EndFrame( void )
{
	PL_GET_CVAR( "input.mlook", mouseLook );
	if ( mouseLook == NULL || !mouseLook->b_value )
		return;

	int w, h;
	ogeShellInterface_GetWindowSize( &w, &h );
	YnCore_ShellInterface_SetMousePosition( w / 2, h / 2 );
}

unsigned int YnCore_Input_GetNumControllers( void ) { return numControllers; }

YNCoreInputState YnCore_Input_GetButtonStatus( unsigned int slot, YNCoreInputButton button )
{
	assert( slot < CLIENT_INPUT_MAX_CONTROLLERS );
	if ( slot >= CLIENT_INPUT_MAX_CONTROLLERS )
		return YN_CORE_INPUT_STATE_NONE;

	return controllers[ slot ].buttons[ button ];
}

PLVector2 YnCore_Input_GetStickStatus( unsigned int slot, unsigned int stickNum )
{
	assert( slot < CLIENT_INPUT_MAX_CONTROLLERS );
	if ( slot >= CLIENT_INPUT_MAX_CONTROLLERS )
		return pl_vecOrigin2;

	return ( stickNum == 0 ) ? controllers[ slot ].stickL : controllers[ slot ].stickR;
}

void YnCore_Input_RegisterAction( const char *id,
                                  YNCoreInputButton buttons[], unsigned int numDefaultButtons,
                                  YNCoreInputKey keys[], unsigned int numDefaultKeys,
                                  ClientInputActionCallback actionCallback )
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

	if ( numDefaultButtons > YN_CORE_MAX_BUTTON_INPUTS )
	{
		numDefaultButtons = YN_CORE_MAX_BUTTON_INPUTS;
		PRINT_WARNING( "Too many default button inputs for action!\n" );
	}
	if ( numDefaultKeys > YN_CORE_MAX_KEY_INPUTS )
	{
		numDefaultKeys = YN_CORE_MAX_KEY_INPUTS;
		PRINT_WARNING( "Too many default key inputs for action!\n" );
	}

	ClientInputAction *inputAction = PL_NEW( ClientInputAction );
	snprintf( inputAction->id, sizeof( inputAction->id ), "%s", id );
	inputAction->callback = actionCallback;

	memcpy( inputAction->buttons, buttons, sizeof( YNCoreInputButton ) * numDefaultButtons );
	inputAction->numButtonBinds = numDefaultButtons;

	memcpy( inputAction->keys, keys, sizeof( YNCoreInputKey ) * numDefaultKeys );
	inputAction->numKeyBinds = numDefaultKeys;

	inputAction->node = PlInsertLinkedListNode( actionableList, inputAction );
}
