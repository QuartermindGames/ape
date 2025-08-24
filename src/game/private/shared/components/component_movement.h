// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

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

void game_component_movement_tick_( GameMovementComponent *self, ApeEntity *entity, double delta );
