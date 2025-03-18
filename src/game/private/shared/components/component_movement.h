// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct GameMovementComponent
{
	PLVector3 velocity;
} GameMovementComponent;

void game_component_movement_handle_( GameMovementComponent *self, ApeEntity *entity, const PLVector3 *dir );
