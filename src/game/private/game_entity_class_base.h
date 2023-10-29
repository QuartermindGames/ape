// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#define GAME_ENTITY_CLASS_BASE_MAGIC PL_MAGIC_TO_NUM( 'B', 'A', 'S', 'E' )

typedef enum GameEntityAttackStyle
{
	GAME_ENTITY_ATTACK_STYLE_NONE,
	GAME_ENTITY_ATTACK_STYLE_EVASIVE,
	GAME_ENTITY_ATTACK_STYLE_STAND_GROUND,
	GAME_ENTITY_ATTACK_STYLE_DIRECT,
} GameEntityAttackStyle;

typedef struct GameEntityClassBase
{
	unsigned int magic;// 'BASE'

	int lodDistances[ 3 ];

	PLCollisionAABB boundingBox;
	PLCollisionSphere sphere;
	float collisionRadius;

	float animationScale;

	int life;
	int armor;
	GameEntityFlag flags;
	GameAIFlag aiFlags;
} GameEntityClassBase;
