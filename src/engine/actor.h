/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

typedef struct NLNode NLNode;// common/node

typedef enum ActorType
{
	ACTOR_NONE,
	ACTOR_PLAYER,
	ACTOR_LIGHT,
	ACTOR_TRIGGER_VOLUME,

	// qciaj 2021
	ACTOR_SG_SHIP,
	ACTOR_SG_ASTEROID,
	ACTOR_SG_ASTEROID_MANAGER,
	ACTOR_SG_PROJECTILE,
	ACTOR_SG_PROP,

	MAX_ACTOR_TYPES
} ActorType;

typedef enum ActorMovementType
{
	ACTOR_MOVEMENT_CUSTOM,
	ACTOR_MOVEMENT_PHYSICS,

	ACTOR_MOVEMENT_SG,

	MAX_ACTOR_MOVEMENT_TYPES
} ActorMovementType;

typedef enum ActorCollisionGroup
{
	PL_BITFLAG( ACTOR_COLLISION_GROUP_WORLD, 0U ),
	PL_BITFLAG( ACTOR_COLLISION_GROUP_PLAYER, 1U ),
	PL_BITFLAG( ACTOR_COLLISION_GROUP_MONSTER, 2U ),
} ActorCollisionGroup;

typedef struct Actor Actor;
typedef struct ActorSetup
{
	const char *id;
	void ( *Spawn )( Actor *self );
	void ( *Tick )( Actor *self, void *userData );
	void ( *Draw )( Actor *self, void *userData );
	void ( *Collide )( Actor *self, Actor *other, void *userData );
	void ( *Destroy )( Actor *self, void *userData );

	NLNode *( *Serialize )( Actor *self, NLNode *nodeTree );
	void ( *Deserialize )( Actor *self, NLNode *nodeTree );
} ActorSetup;
#define ACTOR_SETUP( ID, SPAWN, TICK, DRAW, COLLIDE, DESTROY, SERIALIZE, DESERIALIZE ) \
	ActorSetup##ID { #ID, SPAWN, TICK, DRAW, COLLIDE, DESTROY, SERIALIZE, DESERIALIZE }

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

	/* collision/vis */
	struct WorldSector *sector;
	ActorMovementType movementType;
	ActorCollisionGroup collisionGroup;
	PLCollisionAABB collisionVolume;
	PLCollisionAABB visibilityVolume;
	PLLinkedList *geoColliders; /* list of faces we're touching to test against */

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

	PLLinkedListNode *node;
	void *userData;
} Actor;

typedef struct ActInterface
{
	void ( *RegisterActorType )( ActorSetup *actorSetup );

	Actor *( *SpawnActor )( const char *id, const PLVector3 *position, const PLVector3 *angles );
	Actor *( *DestroyActor )( Actor *self );

	void ( *SetPosition )( Actor *self, const PLVector3 *position );
} ActInterface;

void Act_Initialize( void );
void Act_Shutdown( void );

void Act_RegisterActorType( ActorSetup *actorSetup );

void Act_DrawActors( void );
void Act_TickActors( void *userData, double delta );

Actor *Act_SpawnActor( ActorType type, NLNode *nodeTree );
Actor *Act_SpawnActorById( const char *id, NLNode *nodeTree );
Actor *Act_DestroyActor( Actor *self );
void Act_DestroyActors( void );

ActorType Act_GetType( const Actor *self );

void Act_SetPosition( Actor *self, const PLVector3 *position );
PLVector3 Act_GetPosition( const Actor *self );

void Act_SetVelocity( Actor *self, const PLVector3 *velocity );
PLVector3 Act_GetVelocity( const Actor *self );

void Act_SetAngle( Actor *self, float angle );
float Act_GetAngle( const Actor *self );

void Act_SetAngles( Actor *self, const PLVector3 *angles );
PLVector3 Act_GetAngles( const Actor *self );

struct WorldSector *Act_GetWorldSector( Actor *self );
void Act_SetWorldSector( Actor *self, struct WorldSector *sector );

void Act_SetViewPitch( Actor *self, float viewPitch );
float Act_GetViewPitch( const Actor *self );

void Act_SetUserData( Actor *self, void *userData );
void *Act_GetUserData( Actor *self );

void Act_SetCurrentFrame( Actor *self, unsigned int frame );
unsigned int Act_GetCurrentFrame( const Actor *self );

void Act_SetViewOffset( Actor *self, float viewOffset );
float Act_GetViewOffset( Actor *self );

void Act_SetBounds( Actor *self, PLVector3 mins, PLVector3 maxs );
const PLCollisionAABB *Act_GetBounds( Actor *self );
bool Act_IsColliding( Actor *self, Actor *other );
Actor *Act_CheckCollisions( Actor *self );

void Act_SetVisibilityVolume( Actor *self, const PLVector3 *mins, const PLVector3 *maxs );
bool Act_IsVisible( Actor *self );

PLVector3 Act_GetForward( const Actor *self );

Actor *Act_GetByTag( const char *tag, Actor *start );

/* generic monster functions */
void Monster_Collide( struct Actor *self, struct Actor *other, float force );

/* player functions */
struct Camera *Player_GetCamera( Actor *self );
