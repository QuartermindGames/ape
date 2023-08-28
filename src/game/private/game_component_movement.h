// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "game_private.h"

typedef struct GameMovementComponent {
	float forwardVelocity;
	float strafeVelocity;

	PLVector3 velocity;

	float maxRunSpeed, maxWalkSpeed;

	ApeEntityComponent *inputComponent;
	ApeEntityComponent *cameraComponent;
} GameMovementComponent;
#define GAME_MOVEMENT_COMPONENT( A ) ( ( GameMovementComponent * ) ( A ) )

const ApeEntityComponentCallbackTable *Game_Component_Movement_GetCallbackTable( void );
