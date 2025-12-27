// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/**
 * Initialize the Discord integration.
 *
 * @param clientId 	Your auth ID for your application.
 * @return			False on fail and true on success. On fail, automatically spits details to console.
 */
bool game_integrations_discord_initialize_( int64_t clientId );

/**
 * Shutdown the Discord integration.
 */
void game_integrations_discord_shutdown_();

/**
 * Update the displayed status/activity per Discord.
 *
 * @param details 	Details of the current activity.
 * @param state 	The state in relation to the activity. Can be null.
 * @param image 	App image to use.
 * @param imageText App image text to use. Can be null.
 */
void game_integrations_discord_update_activity_( const char *details, const char *state, const char *image, const char *imageText );

/**
 * Tick the Discord integration.
 * Does nothing if the integration failed to init.
 *
 * @return	False if the integration fails for some reason, otherwise true.
 */
bool game_integrations_discord_tick_();
