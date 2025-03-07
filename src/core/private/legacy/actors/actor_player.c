// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "../actor.h"

#include "renderer/renderer.h"

#define PLAYER_VIEW_OFFSET   0.0f
#define PLAYER_CROUCH_OFFSET 4.0f

#define PLAYER_TURN_SPEED   2.0f
#define PLAYER_WALK_SPEED   0.25f
#define PLAYER_RUN_SPEED    0.75f
#define PLAYER_MAX_VELOCITY PLAYER_RUN_SPEED
#define PLAYER_MIN_VELOCITY 0.5f

#define PLAYER_MAX_PITCH 85.0f
#define PLAYER_MIN_PITCH -85.0f

#define PLAYER_BOUNDS_MAXS PL_VECTOR3( 16.0f, 90.0f, 16.0f )
#define PLAYER_BOUNDS_MINS PL_VECTOR3( -16.0f, 0.0f, -16.0f )

typedef struct APlayer
{
	PLVector3 centerView; /* center */

	float forwardVelocity;
	float strafeVelocity;

	float viewBob;

	PLVector3 viewAngles;

	ApeCamera *eyeCamera;

	//PLMModel *model;
} APlayer;

#define APLAYER( X ) ( ( APlayer * ) ( X )->userData )

static void Player_ApplyViewBob( Actor *self )
{
	/* apply view bob */
	float velocityVector = PlVector3Length( self->velocity );
	APLAYER( self )->viewBob += ( sinf( ape_get_num_ticks() / 5.0f ) / 10.0f ) * velocityVector;

	float viewOffset = self->position.y + PLAYER_VIEW_OFFSET;
	if ( ss_shell_get_key_state( 'c' ) )
		viewOffset = self->position.y + PLAYER_CROUCH_OFFSET;

	self->viewOffset = viewOffset + APLAYER( self )->viewBob;
}

static void Player_HandleMouseLook( Actor *self )
{
	PL_GET_CVAR( "input/mlook", mouseLook );
	if ( mouseLook == NULL || !mouseLook->b_value )
		return;

	int mx, my;
	ape_client_input_get_mouse_delta( &mx, &my );

	self->angles.y += mx;

	// Update and clamp the players view pitch
	self->viewPitch += my;
	self->viewPitch = PlClamp( PLAYER_MIN_PITCH, self->viewPitch, PLAYER_MAX_PITCH );
}

static void Player_Tick( Actor *self, void *userData )
{
	if ( ss_shell_get_button_state( INPUT_A ) )
		self->velocity.y += 10.0f;

	Player_HandleMouseLook( self );

	static const float incAmount = 0.25f;

	// Forward/backward
	if ( ss_shell_get_button_state( APE_INPUT_UP ) || ss_shell_get_key_state( 'w' ) )
		APLAYER( self )->forwardVelocity += incAmount;
	else if ( ss_shell_get_button_state( APE_INPUT_DOWN ) || ss_shell_get_key_state( 's' ) )
		APLAYER( self )->forwardVelocity -= incAmount;
	else if ( APLAYER( self )->forwardVelocity != 0.0f )
	{
		APLAYER( self )->forwardVelocity = APLAYER( self )->forwardVelocity > 0 ? APLAYER( self )->forwardVelocity - incAmount : APLAYER( self )->forwardVelocity + incAmount;
		if ( APLAYER( self )->forwardVelocity < 0.1f && APLAYER( self )->forwardVelocity > -0.1f )
			APLAYER( self )->forwardVelocity = 0.0f;
	}

	// Strafing
	if ( ss_shell_get_key_state( 'a' ) )
		APLAYER( self )->strafeVelocity += incAmount;
	else if ( ss_shell_get_key_state( 'd' ) )
		APLAYER( self )->strafeVelocity -= incAmount;
	else if ( APLAYER( self )->strafeVelocity != 0.0f )
	{
		APLAYER( self )->strafeVelocity = APLAYER( self )->strafeVelocity > 0 ? APLAYER( self )->strafeVelocity - incAmount : APLAYER( self )->strafeVelocity + incAmount;
		if ( APLAYER( self )->strafeVelocity < 0.1f && APLAYER( self )->strafeVelocity > -0.1f )
			APLAYER( self )->strafeVelocity = 0.0f;
	}

	/* clamp the velocity as necessary */
	float maxVelocity                = ss_shell_get_button_state( INPUT_LEFT_STICK ) || ss_shell_get_key_state( KEY_LEFT_SHIFT ) ? PLAYER_RUN_SPEED : PLAYER_WALK_SPEED;
	APLAYER( self )->forwardVelocity = PlClamp( -maxVelocity, APLAYER( self )->forwardVelocity, maxVelocity );
	APLAYER( self )->strafeVelocity  = PlClamp( -maxVelocity, APLAYER( self )->strafeVelocity, maxVelocity );

	APLAYER( self )->viewAngles   = self->angles;
	APLAYER( self )->viewAngles.z = self->viewPitch;

	PLVector3 left;
	PlAnglesAxes( APLAYER( self )->viewAngles, &left, NULL, &self->forward );

	self->velocity = PlAddVector3( self->velocity,
	                               PlAddVector3(
	                                       PlScaleVector3F( self->forward, APLAYER( self )->forwardVelocity ),
	                                       PlScaleVector3F( left, APLAYER( self )->strafeVelocity ) ) );

	//Player_CalculateViewFrustum( self );
}
