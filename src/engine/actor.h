/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

typedef struct NLNode NLNode;// common/node

typedef enum ActorType
{
	ACTOR_NONE,
	ACTOR_PLAYER,
	ACTOR_LIGHT,
	ACTOR_TRIGGER_VOLUME,

	MAX_ACTOR_TYPES
} ActorType;

typedef enum ActorMovementType
{
	ACTOR_MOVEMENT_CUSTOM,
	ACTOR_MOVEMENT_PHYSICS,

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
	void ( *Spawn )( struct Actor *self );
	void ( *Tick )( struct Actor *self, void *userData );
	void ( *Draw )( struct Actor *self, void *userData );
	void ( *Collide )( struct Actor *self, struct Actor *other, void *userData );
	void ( *Destroy )( struct Actor *self, void *userData );

	NLNode *( *Serialize )( struct Actor *self, NLNode *nodeTree );
	void ( *Deserialize )( struct Actor *self, NLNode *nodeTree );
} ActorSetup;

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

void Act_SpawnActors( const char *worldPath );
void Act_DrawActors( void );
void Act_TickActors( void *userData, double delta );

Actor *Act_SpawnActor( const char *id, const PLVector3 *position, const PLVector3 *angles );
Actor *Act_DestroyActor( Actor *self );

ActorType Act_GetType( const Actor *self );

void      Act_SetPosition( Actor *self, const PLVector3 *position );
PLVector3 Act_GetPosition( const Actor *self );

void      Act_SetVelocity( Actor *self, const PLVector3 *velocity );
PLVector3 Act_GetVelocity( const Actor *self );

void  Act_SetAngle( Actor *self, float angle );
float Act_GetAngle( const Actor *self );

void  Act_SetViewPitch( Actor *self, float viewPitch );
float Act_GetViewPitch( const Actor *self );

void  Act_SetUserData( Actor *self, void *userData );
void *Act_GetUserData( Actor *self );

void         Act_SetCurrentFrame( Actor *self, unsigned int frame );
unsigned int Act_GetCurrentFrame( const Actor *self );

void  Act_SetViewOffset( Actor *self, float viewOffset );
float Act_GetViewOffset( Actor *self );

void                   Act_SetBounds( Actor *self, PLVector3 mins, PLVector3 maxs );
const PLCollisionAABB *Act_GetBounds( Actor *self );

PLVector3 Act_GetForward( const Actor *self );

/* generic monster functions */
void Monster_Collide( struct Actor *self, struct Actor *other, void *userData );

/* player functions */
struct GfxCamera *Player_GetCamera( Actor *self );
bool              Player_IsPointVisible( Actor *self, const PLVector2 *point );
