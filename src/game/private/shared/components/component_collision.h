// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef enum GameCollisionGroup : uint8_t
{
	PL_BITFLAG( GAME_COLLISION_GROUP_WORLD, 0U ),
	PL_BITFLAG( GAME_COLLISION_GROUP_PLAYER, 1U ),
	PL_BITFLAG( GAME_COLLISION_GROUP_MONSTER, 2U ),
} GameCollisionGroup;

typedef enum GameCollisionType : uint8_t
{
	GAME_COLLISION_TYPE_SPHERE,
	GAME_COLLISION_TYPE_AABB,
	GAME_COLLISION_TYPE_CAPSULE,
} GameCollisionType;

#define GAME_COLLISION_MAX_COLLIDERS 16

typedef struct GameCollisionComponent
{
	GameCollisionGroup groups;
	GameCollisionType  type;

	union
	{
		PLCollisionSphere   sphere;
		PLCollisionAABB     aabb;
		ComCollisionCapsule capsule;
	};
} GameCollisionComponent;
