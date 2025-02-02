// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define GAME_MAX_TEAMS 4

typedef char GameTeamName[ 64 ];

typedef struct GameTeam
{
	GameTeamName name;
	UInt         numPlayers;
} GameTeam;

/**
 * Initialize teams.
 */
void game_team_init( UInt teamCount );

/**
 * Get the number of active teams.
 *
 * @return The number of currently active teams.
 */
UInt game_team_get_num_active();

/**
 * Get a desired team by its index.
 *
 * @param index Index of the desired team.
 * @return		Pointer to the desired team if successful (otherwise null).
 */
GameTeam *game_team_get( UInt index );

/**
 * Try to get an available slot on a team.
 *
 * @param player	Player to assign.
 * @return			Index of the player team that's assigned. Returns -1 on fail.
 */
int game_team_assign( GamePlayer *player );

/**
 * Attempts to assign the player to the specified team.
 *
 * @param player	Player to assign.
 * @param teamIndex Index of the desired team.
 * @return			The index on success, otherwise -1 on fail.
 */
int game_team_set( GamePlayer *player, UInt teamIndex );
