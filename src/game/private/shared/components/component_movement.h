// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct GameMovementComponent
{
	PLVector3 direction;

	float forwardVelocity;
	float strafeVelocity;

	PLVector3 velocity;

	PLVector3 contactNormal;

	float maxRunSpeed, maxWalkSpeed;

	bool isGrounded;
} GameMovementComponent;

void game_component_movement_tick_( GameMovementComponent *self, ApeEntity *entity, double delta );
