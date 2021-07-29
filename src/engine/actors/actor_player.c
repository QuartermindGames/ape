/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plmodel/plm.h>

#include "yin.h"
#include "actor.h"

#include "client/renderer/renderer.h"

#define PLAYER_VIEW_OFFSET	 75.0f
#define PLAYER_CROUCH_OFFSET 45.0f

#define PLAYER_TURN_SPEED	2.0f
#define PLAYER_WALK_SPEED	2.0f
#define PLAYER_RUN_SPEED	4.0f
#define PLAYER_MAX_VELOCITY PLAYER_RUN_SPEED
#define PLAYER_MIN_VELOCITY 0.5f

#define PLAYER_MAX_PITCH 85.0f
#define PLAYER_MIN_PITCH -85.0f

#define PLAYER_BOUNDS_MAXS PLVector3( 16.0f, 90.0f, 16.0f )
#define PLAYER_BOUNDS_MINS PLVector3( -16.0f, 0.0f, -16.0f )

typedef struct APlayer
{
	PLVector3 llViewPos;
	PLVector3 lrViewPos;

	PLVector3 centerView; /* center */

	float forwardVelocity;
	float viewBob;

	Camera *eyeCamera;

	PLMModel *model;
} APlayer;

Camera *Player_GetCamera( Actor *self )
{
	APlayer *playerData = Act_GetUserData( self );
	if ( playerData == NULL )
		return NULL;

	return playerData->eyeCamera;
}

static void Player_CalculateViewFrustum( Actor *self )
{
	APlayer *playerData = Act_GetUserData( self );

	PLVector3 forward, left;
	PlAnglesAxes( PLVector3( 0, Act_GetAngle( self ), 0 ), &left, NULL, &forward );

	PLVector3 curPos = Act_GetPosition( self );
	curPos.y += Act_GetViewOffset( self );

	playerData->centerView = PlAddVector3( curPos, PlScaleVector3F( forward, 1000.0f ) );

	playerData->llViewPos = PlAddVector3( curPos, PlScaleVector3F( left, 64.0f ) );
	playerData->lrViewPos = PlSubtractVector3( curPos, PlScaleVector3F( left, 64.0f ) );

	/* in future, set this up properly relative to view */
}

/**
 * Ensure the 2D point provided is forward of the player's position
 */
#if 0// unused
bool Player_IsPointVisible( Actor *self, const PLVector2 *point )
{
	if ( Act_GetType( self ) != ACTOR_PLAYER )
		return false;

	APlayer *playerData = Act_GetUserData( self );
	if ( playerData == NULL )
		return false;

	PLVector2 lineStart = PLVector2( playerData->llViewPos.x, playerData->llViewPos.z );
	PLVector2 lineEnd   = PLVector2( playerData->lrViewPos.x, playerData->lrViewPos.z );

	/* in future, set this up properly relative to view */

	float d = PlTestPointLinePosition( point, &lineStart, &lineEnd );
	if ( d > 0.0f )
		return false;

	return true;
}
#endif

/* move this somewhere else... */
static unsigned int numPlayers = 0;

static void Player_Spawn( Actor *self )
{
	APlayer *playerData = globalSystem.MAlloc( sizeof( APlayer ), true );
	Act_SetUserData( self, playerData );

	playerData->model = PlmLoadModel( "models/test/md2/bird_final.md2" );

	if ( numPlayers == 0 )
	{	/* local player */
		//playerData->eyeCamera              = R_CreateCamera( Act_GetPosition( self ), PLVector3( 0, Act_GetAngle( self ), 0 ) );
		//playerData->eyeCamera->parentActor = self;
	}

	Act_SetBounds( self, PLAYER_BOUNDS_MINS, PLAYER_BOUNDS_MAXS );

	Act_SetViewOffset( self, PLAYER_VIEW_OFFSET );
	Player_CalculateViewFrustum( self );

	numPlayers++;
}

static void Player_Tick( Actor *self, void *userData )
{
	PLVector3 curOrigin	  = Act_GetPosition( self );
	PLVector3 curVelocity = Act_GetVelocity( self );

	float nAngle = Act_GetAngle( self );
	if ( globalSystem.GetButtonState( INPUT_LEFT ) || globalSystem.GetKeyState( 'a' ) )
		nAngle += PLAYER_TURN_SPEED;
	else if ( globalSystem.GetButtonState( INPUT_RIGHT ) || globalSystem.GetKeyState( 'd' ) )
		nAngle -= PLAYER_TURN_SPEED;

	Act_SetAngle( self, nAngle );

	if ( globalSystem.GetButtonState( INPUT_A ) )
		curVelocity.y += 10.0f;

	static const float incAmount  = 0.25f;
	APlayer *		   playerData = ( APlayer * ) userData;
	if ( globalSystem.GetButtonState( INPUT_UP ) || globalSystem.GetKeyState( 'w' ) )
		playerData->forwardVelocity += incAmount;
	else if ( globalSystem.GetButtonState( INPUT_DOWN ) || globalSystem.GetKeyState( 's' ) )
		playerData->forwardVelocity -= incAmount;
	else if ( playerData->forwardVelocity != 0.0f )
	{
		playerData->forwardVelocity = playerData->forwardVelocity > 0 ? playerData->forwardVelocity - incAmount : playerData->forwardVelocity + incAmount;
		if ( playerData->forwardVelocity < 0.1f && playerData->forwardVelocity > -0.1f )
			playerData->forwardVelocity = 0.0f;
	}

	float viewPitch = Act_GetViewPitch( self );
	if ( globalSystem.GetKeyState( 'q' ) )
		viewPitch += 1.0f;
	else if ( globalSystem.GetKeyState( 'e' ) )
		viewPitch -= 1.0f;

	/* clamp the view pitch */
	if ( viewPitch < PLAYER_MIN_PITCH )
		viewPitch = PLAYER_MIN_PITCH;
	else if ( viewPitch > PLAYER_MAX_PITCH )
		viewPitch = PLAYER_MAX_PITCH;

	Act_SetViewPitch( self, viewPitch );

	/* clamp the velocity as necessary */
	float maxVelocity			= globalSystem.GetButtonState( INPUT_LEFT_STICK ) ? PLAYER_RUN_SPEED : PLAYER_WALK_SPEED;
	playerData->forwardVelocity = PlClamp( -maxVelocity, playerData->forwardVelocity, maxVelocity );

	curVelocity = PlAddVector3( curVelocity, PlScaleVector3F( Act_GetForward( self ), playerData->forwardVelocity ) );
	Act_SetVelocity( self, &curVelocity );

	Player_CalculateViewFrustum( self );

	/* apply view bob */
	float velocityVector = PlVector3Length( curVelocity );
	playerData->viewBob += ( sinf( Engine_GetNumTicks() / 5.0f ) / 10.0f ) * velocityVector;

	float viewOffset = curOrigin.y + PLAYER_VIEW_OFFSET;
	if ( globalSystem.GetKeyState( 'c' ) )
		viewOffset = curOrigin.y + PLAYER_CROUCH_OFFSET;

	Act_SetViewOffset( self, viewOffset /*+ playerData->viewBob*/ );
	Act_SetPosition( self, &curOrigin );
}

static void Player_Draw( Actor *self, void *userData )
{
	APlayer *playerData = ( APlayer * ) userData;
	if ( playerData->model == NULL )
		return;

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();
	PlTranslateMatrix( Act_GetPosition( self ) );

	for ( unsigned int i = 0; i < playerData->model->numMeshes; ++i )
		RM_DrawMesh( RM_GetFallbackMaterial(), playerData->model->meshes[ i ] );

	PlPopMatrix();
}

static void Player_Collide( Actor *self, Actor *other, void *userData )
{
	Monster_Collide( self, other, userData );

	APlayer *playerData			= ( APlayer * ) userData;
	playerData->forwardVelocity = ( playerData->forwardVelocity / 2.0f ) * -1.0f;
}

const ActorSetup actorPlayerSetup = {
		.id			 = "point.player",
		.Spawn		 = Player_Spawn,
		.Tick		 = Player_Tick,
		.Draw		 = Player_Draw,
		.Collide	 = Player_Collide,
		.Destroy	 = NULL,
		.Serialize	 = NULL,
		.Deserialize = NULL,
};
