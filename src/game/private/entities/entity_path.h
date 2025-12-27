// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

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
