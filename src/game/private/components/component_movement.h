// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "component_collision.h"

typedef struct GameMovementComponent
{
	QmMathVector3f direction;

	float forwardVelocity;
	float strafeVelocity;

	QmMathVector3f velocity;

	QmMathVector3f contactNormal;

	float maxRunSpeed, maxWalkSpeed;

	bool isGrounded;
} GameMovementComponent;

void game_component_movement_tick_( GameMovementComponent *self, GameCollisionComponent *collision, ApeEntity *entity, double delta );
