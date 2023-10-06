// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Questing functionality
// Author:  Mark E. Sowden

#pragma once

typedef enum ToxQuestState
{
	TOX_QUEST_STATE_NONE,     // Quest is pending in the background
	TOX_QUEST_STATE_MENTIONED,// A character has notified the player
	TOX_QUEST_STATE_ASSIGNED, // Has taken on the quest
	TOX_QUEST_STATE_COMPLETED,// Completed it successfully
	TOX_QUEST_STATE_FAILED,   // Botched it
} ToxQuestState;

typedef struct ToxQuestObjective
{
	char description[ 256 ];

	bool hasTimeRange;
	unsigned int availableTimeRange[ 2 ];// in total seconds relative to current day
} ToxQuestObjective;

typedef struct ToxQuest
{
	char title[ 128 ];
	char description[ 256 ];

	PLGTexture *icon;

	bool hasTimeRange;
	unsigned int availableTimeRange[ 2 ];// in total seconds relative to current day
} ToxQuest;
