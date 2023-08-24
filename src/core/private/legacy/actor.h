/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

typedef struct NdBranch NdBranch;// common/node

typedef enum ActorType {
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

typedef enum ActorMovementType {
	ACTOR_MOVEMENT_PHYSICS,

	MAX_ACTOR_MOVEMENT_TYPES
} ActorMovementType;

typedef enum ActorCollisionGroup {
	PL_BITFLAG( ACTOR_COLLISION_GROUP_WORLD, 0U ),
	PL_BITFLAG( ACTOR_COLLISION_GROUP_PLAYER, 1U ),
	PL_BITFLAG( ACTOR_COLLISION_GROUP_MONSTER, 2U ),
} ActorCollisionGroup;

typedef struct Actor Actor;
typedef struct ActorSetup {
	const char *id;
	void ( *Spawn )( Actor *self );
	void ( *Tick )( Actor *self, void *userData );
	void ( *Draw )( Actor *self, void *userData );
	void ( *Collide )( Actor *self, Actor *other, void *userData );
	void ( *Destroy )( Actor *self, void *userData );

	NdBranch *( *Serialize )( Actor *self, NdBranch *nodeTree );
	void ( *Deserialize )( Actor *self, NdBranch *nodeTree );
} ActorSetup;

typedef struct Actor {
	PLVector3 position, oldPosition;
	PLVector3 angles, oldAngles;
	PLVector3 velocity;
	PLVector3 forward;
	float angle;
	float viewPitch;
	float viewOffset;

	char tagName[ 64 ];

	/* collision/vis */
	struct ApeWorldRoom *sector;
	ActorMovementType movementType;
	ActorCollisionGroup collisionGroup;
	PLCollisionAABB collisionVolume;
	PLCollisionAABB visibilityVolume;
	struct PLLinkedList *geoColliders; /* list of faces we're touching to test against */

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

void Act_DrawActors( ApeCamera *camera, ApeWorldRoom *sector );
void Act_TickActors( void *userData, double delta );

Actor *Act_SpawnActor( ActorType type, NdBranch *nodeTree );
Actor *Act_SpawnActorById( const char *id, NdBranch *nodeTree );
Actor *Act_DestroyActor( Actor *self );

ActorType Act_GetType( const Actor *self );

void Act_SetPosition( Actor *self, const PLVector3 *position );
PLVector3 Act_GetPosition( const Actor *self );

float Act_GetAngle( const Actor *self );

void Act_SetWorldSector( Actor *self, struct ApeWorldRoom *sector );

void Act_SetUserData( Actor *self, void *userData );
void *Act_GetUserData( Actor *self );

void Act_SetViewOffset( Actor *self, float viewOffset );
float Act_GetViewOffset( Actor *self );

void Act_SetBounds( Actor *self, PLVector3 mins, PLVector3 maxs );
bool Act_IsColliding( Actor *self, Actor *other );
Actor *Act_CheckCollisions( Actor *self );

void Act_SetVisibilityVolume( Actor *self, const PLVector3 *mins, const PLVector3 *maxs );
bool Act_IsVisible( Actor *self, ApeCamera *camera );

PLVector3 Act_GetForward( const Actor *self );

Actor *Act_GetByTag( const char *tag, Actor *start );

/* generic monster functions */
void Monster_Collide( struct Actor *self, struct Actor *other, float force );
