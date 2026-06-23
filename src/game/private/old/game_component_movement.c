// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "game_component_movement.h"

static void HandleMouseLook( GameMovementComponent *movementComponent )
{
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( !mouseLook->b_value )
	{
		return;
	}

	int mx, my;
	apeShellInterface_GetMousePosition( &mx, &my );//TODO: should use Client_Input_GetMouseDelta ...

	//TODO
}

static void Tick( ApeEntityComponent *self )
{
	if ( apeShellInterface_GetButtonState( INPUT_A ) )
	{
		GAME_MOVEMENT_COMPONENT( self )->velocity.y += 10.0f;
	}

	HandleMouseLook( GAME_MOVEMENT_COMPONENT( self ) );

	static const float gain = 0.25f;

	if ( apeShellInterface_GetButtonState( APE_INPUT_UP ) || apeShellInterface_GetKeyState( 'w' ) )
	{
		GAME_MOVEMENT_COMPONENT( self )->forwardVelocity += gain;
	}
	else if ( apeShellInterface_GetButtonState( APE_INPUT_DOWN ) || apeShellInterface_GetKeyState( 's' ) )
	{
		GAME_MOVEMENT_COMPONENT( self )->forwardVelocity -= gain;
	}
	else if ( GAME_MOVEMENT_COMPONENT( self )->forwardVelocity != 0.0f )
	{
		GAME_MOVEMENT_COMPONENT( self )->forwardVelocity = GAME_MOVEMENT_COMPONENT( self )->forwardVelocity > 0 ? GAME_MOVEMENT_COMPONENT( self )->forwardVelocity - gain : GAME_MOVEMENT_COMPONENT( self )->forwardVelocity + gain;
		if ( GAME_MOVEMENT_COMPONENT( self )->forwardVelocity < 0.1f && GAME_MOVEMENT_COMPONENT( self )->forwardVelocity > -0.1f )
		{
			GAME_MOVEMENT_COMPONENT( self )->forwardVelocity = 0.0f;
		}
	}

	// strafing
	if ( apeShellInterface_GetKeyState( 'a' ) )
	{
		GAME_MOVEMENT_COMPONENT( self )->strafeVelocity += gain;
	}
	else if ( apeShellInterface_GetKeyState( 'd' ) )
	{
		GAME_MOVEMENT_COMPONENT( self )->strafeVelocity -= gain;
	}
	else if ( GAME_MOVEMENT_COMPONENT( self )->strafeVelocity != 0.0f )
	{
		GAME_MOVEMENT_COMPONENT( self )->strafeVelocity = GAME_MOVEMENT_COMPONENT( self )->strafeVelocity > 0 ? GAME_MOVEMENT_COMPONENT( self )->strafeVelocity - gain : GAME_MOVEMENT_COMPONENT( self )->strafeVelocity + gain;
		if ( GAME_MOVEMENT_COMPONENT( self )->strafeVelocity < 0.1f && GAME_MOVEMENT_COMPONENT( self )->strafeVelocity > -0.1f )
		{
			GAME_MOVEMENT_COMPONENT( self )->strafeVelocity = 0.0f;
		}
	}

	// clamp the velocity as necessary
	float maxVelocity = apeShellInterface_GetButtonState( INPUT_LEFT_STICK ) || apeShellInterface_GetKeyState( KEY_LEFT_SHIFT ) ? GAME_MOVEMENT_COMPONENT( self )->maxRunSpeed : GAME_MOVEMENT_COMPONENT( self )->maxWalkSpeed;
	GAME_MOVEMENT_COMPONENT( self )->forwardVelocity = PlClamp( -maxVelocity, GAME_MOVEMENT_COMPONENT( self )->forwardVelocity, maxVelocity );
	GAME_MOVEMENT_COMPONENT( self )->strafeVelocity = PlClamp( -maxVelocity, GAME_MOVEMENT_COMPONENT( self )->strafeVelocity, maxVelocity );

#if 0
	QmMathVector3f left;
	PlAnglesAxes( GAME_MOVEMENT_COMPONENT( self )->viewAngles, &left, NULL, &self->forward );
	GAME_MOVEMENT_COMPONENT( self )->velocity = qm_math_vector3f_add( GAME_MOVEMENT_COMPONENT( self )->velocity,
	                               qm_math_vector3f_add(
	                                       qm_math_vector3f_scale_float( GAME_MOVEMENT_COMPONENT( self )->forward, GAME_MOVEMENT_COMPONENT( self )->forwardVelocity ),
	                                       qm_math_vector3f_scale_float( left, GAME_MOVEMENT_COMPONENT( self )->strafeVelocity ) ) );
#endif
}

const ApeEntityComponentCallbackTable *Game_Component_Movement_GetCallbackTable( void )
{
	static ApeEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );

	callbackTable.tickFunction = Tick;

	return &callbackTable;
}
