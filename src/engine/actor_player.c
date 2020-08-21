/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "actor.h"
#include "renderer/renderer.h"

#define PLAYER_VIEW_OFFSET  75.0f

#define PLAYER_TURN_SPEED    2.0f
#define PLAYER_WALK_SPEED    5.0f
#define PLAYER_RUN_SPEED     8.0f
#define PLAYER_MAX_VELOCITY  PLAYER_RUN_SPEED
#define PLAYER_MIN_VELOCITY  0.5f

typedef struct APlayer {
	PLVector3 ulViewPos;
	PLVector3 urViewPos;
	PLVector3 llViewPos;
	PLVector3 lrViewPos;

	PLVector3 centerView; /* center */

	float forwardVelocity;
	float viewBob;

	GfxCamera *eyeCamera;
} APlayer;

GfxCamera *Player_GetCamera( Actor *self ) {
	APlayer *playerData = Act_GetUserData( self );
	if ( playerData == NULL ) {
		return NULL;
	}

	return playerData->eyeCamera;
}

static void Player_CalculateViewFrustum( Actor *self ) {
	APlayer *playerData = Act_GetUserData( self );

	PLVector3 forward, left;
	plAnglesAxes( PLVector3( 0, Act_GetAngle( self ), 0 ), &left, NULL, &forward );

	PLVector3 curPos = Act_GetPosition( self );
	curPos.y += Act_GetViewOffset( self );

	playerData->centerView = plAddVector3( curPos, plScaleVector3f( forward, 1000.0f ) );

	playerData->llViewPos = plAddVector3( curPos, plScaleVector3f( left, 64.0f ) );
	playerData->lrViewPos = plSubtractVector3( curPos, plScaleVector3f( left, 64.0f ) );

	/* in future, set this up properly relative to view */
}

/**
 * Ensure the 2D point provided is forward of the player's position
 */
bool Player_IsPointVisible( Actor *self, const PLVector2 *point ) {
	if ( Act_GetType( self ) != ACTOR_PLAYER ) {
		return false;
	}

	APlayer *playerData = Act_GetUserData( self );
	if ( playerData == NULL ) {
		return false;
	}

	PLVector2 lineStart = PLVector2( playerData->llViewPos.x, playerData->llViewPos.z );
	PLVector2 lineEnd = PLVector2( playerData->lrViewPos.x, playerData->lrViewPos.z );

	/* in future, set this up properly relative to view */

	float d = plTestPointLinePosition( point, &lineStart, &lineEnd );
	if( d > 0.0f ) {
		return false;
	}

	return true;
}

/* move this somewhere else... */
static unsigned int numPlayers = 0;

void Player_Spawn( Actor *self ) {
	APlayer* playerData = g_system.calloc( 1, sizeof( APlayer ) );
	Act_SetUserData( self, playerData );

	if ( numPlayers == 0 ) { /* local player */
		playerData->eyeCamera = Gfx_CreateCamera( VIEW_PERSPECTIVE_EYE, Act_GetPosition( self ), PLVector3( 0, Act_GetAngle( self ), 0 ), Engine_GetMainWindow() );
		playerData->eyeCamera->parentActor = self;
	}

	Act_SetViewOffset( self, PLAYER_VIEW_OFFSET );
	Player_CalculateViewFrustum( self );

	numPlayers++;
}

void Player_Tick( Actor *self, void *userData ) {
	float nAngle = Act_GetAngle( self );
	if ( g_system.GetButtonState( INPUT_LEFT ) ) {
		nAngle += PLAYER_TURN_SPEED;
	} else if ( g_system.GetButtonState( INPUT_RIGHT ) ) {
		nAngle -= PLAYER_TURN_SPEED;
	}
	Act_SetAngle( self, nAngle );

	static const float incAmount = 0.25f;
	APlayer *playerData = ( APlayer * ) userData;
	if ( g_system.GetButtonState( INPUT_UP ) ) {
		playerData->forwardVelocity += incAmount;
	} else if ( g_system.GetButtonState( INPUT_DOWN ) ) {
		playerData->forwardVelocity -= incAmount;
	} else if ( playerData->forwardVelocity != 0.0f ) {
		playerData->forwardVelocity = playerData->forwardVelocity > 0 ? playerData->forwardVelocity - incAmount : playerData->forwardVelocity + incAmount;
		if( playerData->forwardVelocity < 0.1f && playerData->forwardVelocity > -0.1f ) {
			playerData->forwardVelocity = 0.0f;
		}
	}

	/* clamp the velocity as necessary */
	float maxVelocity = g_system.GetButtonState( INPUT_LEFT_STICK ) ? PLAYER_RUN_SPEED : PLAYER_WALK_SPEED;
	playerData->forwardVelocity = plClamp( -maxVelocity, playerData->forwardVelocity, maxVelocity );

	PLVector3 curVelocity = Act_GetVelocity( self );
	curVelocity = plScaleVector3f( Act_GetForward( self ), playerData->forwardVelocity );
	Act_SetVelocity( self, &curVelocity );

	Player_CalculateViewFrustum( self );

	/* apply view bob */
	float velocityVector = plVector3Length( &curVelocity );
	playerData->viewBob += ( sinf( Engine_GetNumTicks() / 5.0f ) / 10.0f ) * velocityVector;
	Act_SetViewOffset( self, PLAYER_VIEW_OFFSET + playerData->viewBob );
}

void Player_Collide( Actor *self, Actor *other, void *userData ) {
	Monster_Collide( self, other, userData );

	APlayer *playerData = (APlayer *)userData;
	playerData->forwardVelocity = ( playerData->forwardVelocity / 2.0f ) * -1.0f;
}
