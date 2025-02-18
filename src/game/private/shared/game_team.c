// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Team management foundation.
// Author:  Mark E. Sowden

#include "game_private.h"
#include "game_team.h"

static GameTeam teams[ GAME_MAX_TEAMS ];
static UInt     numActiveTeams;
static UInt     maxPlayersPerTeam;

void game_team_init( UInt teamCount )
{
	PL_ZERO_( teams );

	if ( teamCount > GAME_MAX_TEAMS )
	{
		game_warning_( "Invalid team count passed (%u)!\n", teamCount );
		teamCount = GAME_MAX_TEAMS;
	}

	numActiveTeams = teamCount;
	if ( teamCount > 1 )
	{
		maxPlayersPerTeam = GAME_MAX_PLAYERS / teamCount;
	}
	else
	{
		maxPlayersPerTeam = 0;
	}
}

void game_team_set_resource_pools( UInt index, UInt *resourcePools, UInt numResourcePools )
{
	GameTeam *team = game_team_get( index );
	if ( team == nullptr )
	{
		return;
	}

	team->resourcePools    = resourcePools;
	team->numResourcePools = numResourcePools;
}

UInt *game_team_get_resource_pools( UInt index, UInt *numResourcePools )
{
	GameTeam *team = game_team_get( index );
	if ( team == nullptr )
	{
		return nullptr;
	}

	*numResourcePools = team->numResourcePools;
	return team->resourcePools;
}

UInt game_team_get_num_active()
{
	return numActiveTeams;
}

GameTeam *game_team_get( UInt index )
{
	if ( index >= GAME_MAX_TEAMS )
	{
		game_warning_( "Invalid team index passed (%u)!\n", index );
		return nullptr;
	}
	return &teams[ index ];
}

int game_team_assign( GamePlayer *player )
{
	// search through for the team with the fewest for now
	bool isUniform = false;
	UInt teamIndex = 0;
	for ( UInt i = 1; i < numActiveTeams; ++i )
	{
		if ( teams[ i ].numPlayers < teams[ teamIndex ].numPlayers )
		{
			teamIndex = i;
		}
	}

	if ( teams[ teamIndex ].numPlayers >= maxPlayersPerTeam )
	{
		game_warning_( "Team %u is full!\n", teamIndex );
		return -1;
	}

	// if all the teams have the same player count,
	// try throwing them to a random team instead
	if ( isUniform )
	{
		int seed = 0;
		for ( ;; )
		{
			teamIndex = PlGenerateRandomIntegerLCG_VB6( &seed ) % numActiveTeams;
			if ( teams[ teamIndex ].numPlayers < maxPlayersPerTeam )
			{
				break;
			}
		}
	}

	return teamIndex;
}

int game_team_set( GamePlayer *player, UInt teamIndex )
{
	if ( player->team == teamIndex )
	{
		// already on that team, doofus!
		return player->team;
	}

	if ( teamIndex >= GAME_MAX_TEAMS )
	{
		game_warning_( "Invalid team index passed (%u)!\n", teamIndex );
		return -1;
	}

	if ( teams[ teamIndex ].numPlayers >= maxPlayersPerTeam )
	{
		game_warning_( "Team %u is full!\n", teamIndex );
		return -1;
	}

	player->team = teamIndex;
	teams[ teamIndex ].numPlayers++;

	return player->team;
}
