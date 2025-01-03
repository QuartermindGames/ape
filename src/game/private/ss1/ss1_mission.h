// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Mission functionality
// Author:  Mark E. Sowden

#pragma once

typedef enum SS1MissionState
{
	SS1_MISSION_STATE_NONE,     // Mission is pending in the background
	SS1_MISSION_STATE_MENTIONED,// A character has notified the player
	SS1_MISSION_STATE_ASSIGNED, // Has taken on the quest
	SS1_MISSION_STATE_COMPLETED,// Completed it successfully
	SS1_MISSION_STATE_FAILED,   // Botched it
} SS1MissionState;

typedef struct SS1MissionObjective
{
	char description[ 256 ];

	bool         hasTimeRange;
	unsigned int availableTimeRange[ 2 ];// in total seconds relative to current day
} SS1MissionObjective;

typedef struct SS1Mission
{
	char title[ 128 ];
	char description[ 256 ];

	PLGTexture *icon;

	bool         hasTimeRange;
	unsigned int availableTimeRange[ 2 ];// in total seconds relative to current day
} SS1Mission;
