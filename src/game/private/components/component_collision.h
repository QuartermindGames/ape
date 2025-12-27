// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef enum GameCollisionGroup : uint8_t
{
#define GAME_COLLISION_GROUP( NAME, VALUE ) PL_BITFLAG( GAME_COLLISION_GROUP_##NAME, ( APE_COLLISION_GROUP_END + VALUE ) )
	GAME_COLLISION_GROUP( WORLD, 0U ),
	GAME_COLLISION_GROUP( PLAYER, 1U ),
	GAME_COLLISION_GROUP( MONSTER, 2U ),
} GameCollisionGroup;

#define GAME_COLLISION_MAX_COLLIDERS 16

typedef struct GameCollisionComponent
{
	ApeIntegerProperty groups;
	ApeEnumProperty    type;

	union
	{
		PLCollisionSphere   sphere;
		PLCollisionAABB     aabb;
		ComCollisionCapsule capsule;
	} collider;
} GameCollisionComponent;
