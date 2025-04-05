// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct GameMovementComponent
{
	float forwardVelocity;
	float strafeVelocity;

	PLVector3 velocity;

	float maxRunSpeed, maxWalkSpeed;
} GameMovementComponent;

void game_component_movement_tick_( GameMovementComponent *self, ApeEntity *entity, double delta );
