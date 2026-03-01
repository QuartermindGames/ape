// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#define GAME_PLAYER_SPAWN_CLASS_NAME "player_spawn"

typedef struct GamePlayerSpawnEntity
{
	ApeEnumProperty team;

	QmOsLinkedListNode *listNode;
} GamePlayerSpawnEntity;
#define PLAYER_SPAWN_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_PLAYER_SPAWN_CLASS_NAME, GamePlayerSpawnEntity )

QmOsLinkedList *game_player_spawn_get_spawn_points();
