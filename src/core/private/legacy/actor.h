// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct AcmBranch AcmBranch;// common/node

typedef enum ActorType
{
	ACTOR_NONE,
	ACTOR_PLAYER,
	ACTOR_LIGHT,
	ACTOR_TRIGGER_VOLUME,

	// qciaj 2021
	ACTOR_SG_SHIP,
	ACTOR_SG_ASTEROID,
	ACTOR_SG_PROJECTILE,
	ACTOR_SG_PROP,

	MAX_ACTOR_TYPES
} ActorType;

typedef struct Actor Actor;
typedef struct ActorSetup
{
	const char *id;
	void ( *Spawn )( Actor *self );
	void ( *Tick )( Actor *self, void *userData );
	void ( *Draw )( Actor *self, void *userData );
	void ( *Collide )( Actor *self, Actor *other, void *userData );
	void ( *Destroy )( Actor *self, void *userData );

	AcmBranch *( *Serialize )( Actor *self, AcmBranch *nodeTree );
	void ( *Deserialize )( Actor *self, AcmBranch *nodeTree );
} ActorSetup;

typedef struct Actor
{
	PLVector3 position, oldPosition;
	PLVector3 angles, oldAngles;
	PLVector3 velocity;
	PLVector3 forward;
	float angle;
	float viewPitch;
	float viewOffset;

	char tagName[ 64 ];

	/* animation */
	unsigned int currentFrame;
	unsigned int frameSwapTime;

	ActorType type;
	ActorSetup setup;

	struct SGNode *graphNode;

	Actor *parent;

	// temporary
	int16_t health;
	int16_t score;

	struct PLLinkedListNode *node;
	void *userData;
} Actor;
