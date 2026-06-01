// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "component_collision.h"

typedef enum GameMovementCapability : uint16_t
{
	QM_OS_BIT_FLAG( GAME_MOVEMENT_CAPABILITY_CROUCH, 0U ),     // crouching
	QM_OS_BIT_FLAG( GAME_MOVEMENT_CAPABILITY_CROUCH_MOVE, 1U ),// move while crouching
	QM_OS_BIT_FLAG( GAME_MOVEMENT_CAPABILITY_JUMP, 2U ),       // jumping
} GameMovementCapability;

typedef enum GameMovementDirection : uint8_t
{
	GAME_MOVEMENT_DIRECTION_FB,// forward, backward
	GAME_MOVEMENT_DIRECTION_LR,// left, right
	GAME_MOVEMENT_DIRECTION_UD,// up, down

	GAME_MOVEMENT_MAX_DIRECTIONS
} GameMovementDirection;

typedef struct GameMovementComponent
{
	GameMovementCapability capabilities;

	// direction is input intent...
	// x+ forward, x- backward
	// y+ jump, y- swim down
	// z+ left, z- right
	// (something like this, I'm sleepy)
	int8_t directions[ GAME_MOVEMENT_MAX_DIRECTIONS ];
	bool   shiftModifier;

	float forwardVelocity;
	float strafeVelocity;

	QmMathVector3f velocity;

	QmMathVector3f contactNormal;

	float maxRunSpeed, maxWalkSpeed;
	float acceleration;
	float jumpSpeed;

	bool          isGrounded;
	ApeBrushFace *groundedFace;
} GameMovementComponent;

void game_component_movement_tick_( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity, double delta );
