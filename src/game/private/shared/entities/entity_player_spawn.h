// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define GAME_PLAYER_SPAWN_CLASS_NAME "player_spawn"

typedef struct GamePlayerSpawnEntity
{
	unsigned int team;

	PLLinkedListNode *listNode;
} GamePlayerSpawnEntity;
#define PLAYER_SPAWN_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), GAME_PLAYER_SPAWN_CLASS_NAME, GamePlayerSpawnEntity )

PLLinkedList *game_player_spawn_get_spawn_points();
