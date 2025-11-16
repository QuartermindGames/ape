// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

/**
 * Get the first follow entity for the camera.
 * @return First entity for the camera to follow.
 */
ApeEntity *game_entity_path_get_first_();

/**
 * Get the next follow entity.
 * @param self Current follow entity.
 * @return The next follow entity after current.
 */
ApeEntity *game_entity_path_get_next_( ApeEntity *self );
