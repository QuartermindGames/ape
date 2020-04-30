/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

typedef enum ActorType {
	ACTOR_NONE,
	ACTOR_PLAYER,

	MAX_ACTOR_TYPES,
} ActorType;

typedef struct Actor Actor;

void Act_Initialize( void );
void Act_Shutdown( void );

void Act_SpawnActors( void );
void Act_DisplayActors( void );
void Act_TickActors( void );

Actor *Act_SpawnActor( ActorType type, PLVector3 position, float angle );
Actor *Act_DestroyActor( Actor *self );

ActorType    Act_GetType( const Actor *self );
void         Act_SetPosition( Actor *self, const PLVector3 *position );
PLVector3    Act_GetPosition( const Actor *self );
void         Act_SetVelocity( Actor *self, const PLVector3 *velocity );
PLVector3    Act_GetVelocity( const Actor *self );
void         Act_SetAngle( Actor *self, float angle );
float        Act_GetAngle( const Actor *self );
void         Act_SetUserData( Actor *self, void *userData );
void         *Act_GetUserData( Actor *self );
void         Act_SetCurrentFrame( Actor *self, unsigned int frame );
unsigned int Act_GetCurrentFrame( const Actor *self );
void         Act_SetViewOffset( Actor *self, float viewOffset );
float        Act_GetViewOffset( Actor *self );
void         Act_SetBounds( Actor *self, PLVector3 mins, PLVector3 maxs );
const PLAABB *Act_GetBounds( Actor *self );
PLVector3    Act_GetForward( const Actor *self );

/* generic monster functions */
void Monster_Collide( struct Actor *self, struct Actor *other, void *userData );

/* player functions */
bool Player_IsPointVisible( Actor *self, const PLVector2 *point );
