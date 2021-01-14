/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>

#include "yin.h"
#include "actor.h"
#include "renderer/renderer.h"
#include "map.h"

typedef struct ActorSetup {
	void (*Spawn)( struct Actor *self );
	void (*Tick)( struct Actor *self, void *userData );
	void (*Draw)( struct Actor *self, void *userData );
	void (*Collide)( struct Actor *self, struct Actor *other, void *userData );
	void (*Destroy)( struct Actor *self, void *userData );
} ActorSetup;

static void Act_DrawBasic( Actor *self, void *userData ) {
	plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ] );

	Gfx_DrawAxesPivot( Act_GetPosition( self ), PLVector3( 0, 0, 0 ) );
}

void Monster_Collide( struct Actor *self, struct Actor *other, void *userData ) {
	/* decide what direction to push out from */
	PLVector3 pushDir = plSubtractVector3( Act_GetPosition( other ), Act_GetPosition( self ) );
	/* need to this based on distance from center */
	float length = plVector3Length( pushDir ) / 200.0f;
	pushDir = plScaleVector3f( pushDir, length );
	Act_SetVelocity( other, &pushDir );
}

void Player_Spawn( Actor *self );
void Player_Tick( Actor *self, void *userData );
void Player_Collide( Actor *self, Actor *other, void *userData );

ActorSetup actorSpawnSetup[ MAX_ACTOR_TYPES ] = {
		[ ACTOR_NONE   ] = { NULL, NULL, Act_DrawBasic, NULL, NULL },
		[ ACTOR_PLAYER ] = { Player_Spawn, Player_Tick, NULL, Player_Collide, NULL },
       // [ ACTOR_LIGHT ] = { Light_Spawn, Light_Tick, Light_Draw, NULL, NULL },
		//[ ACTOR_BOSS   ] = { Boss_Spawn, Boss_Tick, Boss_Draw, Monster_Collide, NULL },
		//[ ACTOR_SARG   ] = { Sarg_Spawn, Troo_Tick, Sarg_Draw, Monster_Collide, NULL },
		//[ ACTOR_TROO   ] = { Troo_Spawn, Sarg_Tick, Troo_Draw, Monster_Collide, NULL },
};

typedef struct Actor {
	PLVector3       position, oldPosition;
	PLVector3       velocity;
	PLVector3       forward;
	float           angle;
	float           viewPitch;
	float           viewOffset;
	unsigned int    area;

	/* collision/vis */
	ActorMovementType movementType;
	ActorCollisionGroup collisionGroup;
	PLCollisionAABB bounds;
	PLLinkedList *geoColliders; /* list of faces we're touching to test against */

	/* animation */
	unsigned int currentFrame;
	unsigned int frameSwapTime;

	ActorType  type;
	ActorSetup setup;
	
	SGNode *graphNode;

	PLLinkedListNode *node;
	void             *userData;
} Actor;

static PLLinkedList *actorList;

Actor *Act_SpawnActor( ActorType type, PLVector3 position, float angle ) {
	Actor *actor = Sys_calloc( 1, sizeof( Actor ) );
	actor->node     = plInsertLinkedListNode( actorList, actor );
	actor->setup    = actorSpawnSetup[ type ];
	actor->area     = 0;
	actor->type     = type;
	actor->position = position;
	actor->angle    = angle;

	actor->geoColliders = plCreateLinkedList();
	if ( actor->geoColliders == NULL ) {
		PrintError( "Failed to create colliders list!\nPL: %s\n", plGetError() );
	}

	/* give everything a set of basic bounds */
	actor->bounds.maxs = PLVector3( 16.0f, 16.0f, 16.0f );
	actor->bounds.mins = PLVector3( -16.0f, -16.0f, -16.0f );

	if ( actor->setup.Spawn != NULL ) {
		actor->setup.Spawn( actor );
	}

	return actor;
}

Actor *Act_DestroyActor( Actor *self ) {
	if ( self->setup.Destroy != NULL) {
		self->setup.Destroy( self, self->userData );
	}

	plDestroyLinkedList( self->geoColliders );

	plDestroyLinkedListNode( actorList, self->node );
	free( self->userData );
	free( self );
	return NULL;
}

ActorType Act_GetType( const Actor *self ) { return self->type; }
void      Act_SetPosition( Actor *self, const PLVector3 *position ) { self->position = *position; }
PLVector3 Act_GetPosition( const Actor *self ) { return self->position; }
void      Act_SetVelocity( Actor *self, const PLVector3 *velocity ) { self->velocity = *velocity; }
PLVector3 Act_GetVelocity( const Actor *self ) { return self->velocity; }
void      Act_SetAngle( Actor *self, float angle ) { self->angle = angle; }
float     Act_GetAngle( const Actor *self ) { return self->angle; }

/**
 * Up/down view angle, typically used for camera.
 */
void Act_SetViewPitch( Actor *self, float viewPitch ) {
	self->viewPitch = viewPitch;
}

/**
 * Fetch the current up/down view angle.
 */
float Act_GetViewPitch( const Actor *self ) {
	return self->viewPitch;
}

void      Act_SetViewOffset( Actor *self, float viewOffset ) { self->viewOffset = viewOffset; }
float     Act_GetViewOffset( Actor *self ) { return self->viewOffset; }

/**
 * Fetch the actor-specific data.
 */
void Act_SetUserData( Actor *self, void *userData ) {
	self->userData = userData;
}

void      *Act_GetUserData( Actor *self ) { return self->userData; }

void Act_SetCurrentFrame( Actor *self, unsigned int frame ) {
	self->currentFrame = frame;
}

unsigned int Act_GetCurrentFrame( const Actor *self ) {
	return self->currentFrame;
}

void Act_SetBounds( Actor *self, PLVector3 mins, PLVector3 maxs ) {
	if( mins.x > maxs.x || mins.y > maxs.y || mins.z > maxs.z ) {
		PrintError( "Invalid bounds for actor (mins %s, maxs %s)!\n", plPrintVector3( &mins, pl_int_var ), plPrintVector3( &maxs, pl_int_var ) );
	}

	self->bounds.maxs = maxs;
	self->bounds.mins = mins;
}

const PLCollisionAABB *Act_GetBounds( Actor *self ) {
	return &self->bounds;
}

PLVector3 Act_GetForward( const Actor *self ) {
	return self->forward;
}

void Act_SpawnActors( void ) {
	PrintMsg( "Spawning actors...\n" );

#if 0 /* todo: replace */
	PLFile *filePtr = plLoadPackageFile( globalWad, "M_THINGS" );
	if ( filePtr == NULL) {
		PrintError( "Failed to find \"M_THINGS\" block!\nPL: %s\n", plGetError());
	}

	bool status;
	uint32_t numThings = plReadInt32( filePtr, false, &status );
	if ( !status ) {
		PrintError( "Failed to get number of things!\nPL: %s\n", plGetError());
	}

	for ( unsigned int i = 0; i < numThings; ++i ) {
		struct {
			int16_t xPos;
			int16_t yPos;
			uint16_t type;
			uint16_t flags;
		} thing;

		PrintMsg( "Spawning actor %d/%d...\n", i + 1, numThings );

		/* these are intentionally flipped... */
		thing.yPos = ( int16_t ) ( plReadInt32( filePtr, false, &status ) >> 16 ) * 2;
		thing.xPos = ( int16_t ) ( plReadInt32( filePtr, false, &status ) >> 16 ) * 2;
		thing.type = ( uint16_t ) ( plReadInt32( filePtr, false, &status ) >> 16 );
		thing.flags = ( uint16_t ) ( plReadInt32( filePtr, false, &status ) >> 16 );

		if ( !status ) {
			PrintError( "Failed to get thing data!\nPL: %s\n", plGetError());
		}

		Act_SpawnActor( thing.type, PLVector3( thing.xPos, 0, thing.yPos ), 0.0f );
	}
#endif
}

/****************************************
 * COLLISION
 ****************************************/

static bool Act_IsColliding( Actor *self, Actor *other ) {
	if( self->area != other->area ) {
		return false;
	}

	return plIsAABBIntersecting( &self->bounds, &other->bounds );
}

static Actor *Act_CheckCollisions( Actor *self ) {
	/* in the future, perhaps it's worth tracking multiple lists per sector? */
	PLLinkedListNode *curNode = plGetRootNode( actorList );
	while( curNode != NULL ) {
		Actor *actor = plGetLinkedListNodeUserData( curNode );
		if( actor == NULL ) {
			PrintError( "Invalid actor data in node!\n" );
		}

		/* "don't have time to play with myself" */
		if( actor == self ) {
			curNode = plGetNextLinkedListNode( curNode );
			continue;
		}

		if( Act_IsColliding( self, actor ) ) {
			return actor;
		}

		curNode = plGetNextLinkedListNode( curNode );
	}

	return NULL;
}

/****************************************
 * RENDERING
 ****************************************/

void Act_DrawActors( void ) {
	PLLinkedListNode *curNode = plGetRootNode( actorList );
	while ( curNode != NULL ) {
		Actor *actor = plGetLinkedListNodeUserData( curNode );
		if ( actor == NULL ) {
			PrintError( "Invalid actor data in node!\n" );
		}

		if ( actor->setup.Draw ) {
			actor->setup.Draw( actor, actor->userData );
		}

		plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_VERTEX ] );

#if 1
		PLVector3 absOrigin = plGetAABBAbsOrigin( &actor->bounds, actor->position );
		plDrawBoundingVolume( &actor->bounds, PL_COLOUR_BLUE );
		plDrawBoundingVolume( &PLCollisionAABB( absOrigin, PLVector3( -16.0f, -16.0f, -16.0f ), PLVector3( 16.0f, 16.0f, 16.0f ) ), PL_COLOUR_BLUE );

		PLLinkedListNode *colliderNode = plGetRootNode( actor->geoColliders );
		while( colliderNode != NULL ) {
			MapFace *face = plGetLinkedListNodeUserData( colliderNode );

			PLCollisionPlane plane = PLCollisionPlane( face->bounds.absOrigin, plGetPolygonFaceNormal( face->polygon ) );
			PLCollision collision = plIsSphereIntersectingPlane( &PLCollisionSphere( absOrigin, 16.0f ), &plane );
			if ( collision.penetration > 0.0f ) {
				plDrawBoundingVolume( &face->bounds, PL_COLOUR_RED );

				Gfx_DrawAxesPivot( collision.contactPoint, plane.normal );

				PLMatrix4 transform = plMatrix4Identity();
				plDrawSimpleLine( transform, face->bounds.absOrigin, plAddVector3( face->bounds.absOrigin, plScaleVector3f( plane.normal, 64.0f ) ), PLColour( 255, 255, 0, 255 ) );
				plDrawSimpleLine( transform, actor->bounds.origin, collision.contactPoint, PLColour( 0, 255, 0, 255 ) );
			} else {
				plDrawBoundingVolume( &face->bounds, PL_COLOUR_GREEN );
			}

			colliderNode = plGetNextLinkedListNode( colliderNode );
		}
#endif

		curNode = plGetNextLinkedListNode( curNode );
	}
}

#define GRAVITY 4.0f
void Act_TickActors( void ) {
	PLLinkedListNode *curNode = plGetRootNode( actorList );
	while ( curNode != NULL ) {
		Actor *actor = plGetLinkedListNodeUserData( curNode );
		if ( actor == NULL ) {
			PrintError( "Invalid actor data in node!\n" );
		}

		if ( actor->setup.Tick != NULL ) {
			actor->setup.Tick( actor, actor->userData );
		}

		plAnglesAxes( PLVector3( 0, actor->angle, 0 ), NULL, NULL, &actor->forward );

		static const float friction = 4.0f;
		if( actor->velocity.x != 0 ) {
			actor->velocity.x -= ( actor->velocity.x / friction );
		}
		if( actor->velocity.y != 0 ) {
			actor->velocity.y -= ( actor->velocity.y / friction );
		}
		if( actor->velocity.z != 0 ) {
			actor->velocity.z -= ( actor->velocity.z / friction );
		}

		//if ( plGetNumLinkedListNodes( actor->geoColliders ) == 0 ) {
		actor->velocity.y = -GRAVITY;
		//}

		actor->oldPosition = actor->position;
		actor->position = plAddVector3( actor->position, actor->velocity );

		/* check actor vs actor collision */
		if( actor->setup.Collide != NULL ) {
			/* ensure bounds origin is kept updated */
			actor->bounds.origin = actor->position;

			Actor *collider = Act_CheckCollisions( actor );
			if( collider != NULL ) {
				actor->setup.Collide( actor, collider, actor->userData );
			}

			/* and now check actor vs world collision */

			plDestroyLinkedListNodes( actor->geoColliders );

			/* first need to figure out what faces we're intersecting with */
			unsigned int numFaces;
			MapFace *faces = Map_GetFacesForSector( actor->area, &numFaces );
			for ( unsigned int i = 0; i < numFaces; ++i ) {
				if ( !plIsAABBIntersecting( &actor->bounds, &faces[ i ].bounds ) ) {
					continue;
				}

				/* convert the face into a plane */
				PLCollisionPlane plane = PLCollisionPlane( faces[ i ].bounds.absOrigin, plGetPolygonFaceNormal( faces[ i ].polygon ) );

				/* now see if we're hitting anything */
				PLVector3 absOrigin = plGetAABBAbsOrigin( &actor->bounds, actor->position );
				PLCollisionSphere colSphere = PLCollisionSphere( absOrigin, 16.0f );
				PLCollision collision = plIsSphereIntersectingPlane( &colSphere, &plane );
				if ( collision.penetration > 0.0f ) {
					/* printf( "penetration: %f\n", collision.penetration ); */
					actor->position = plAddVector3( actor->position, plScaleVector3f( plNormalizeVector3( collision.contactNormal ), collision.penetration / 2.0f ) );

					PLLinkedListNode *node = plInsertLinkedListNode( actor->geoColliders, &faces[ i ] );
					if ( node == NULL ) {
						PrintError( "Failed to insert node into colliders list!\n" );
					}
				}
			}
		}

		curNode = plGetNextLinkedListNode( curNode );
	}
}

void Act_Initialize( void ) {
	actorList = plCreateLinkedList();
	if ( actorList == NULL) {
		PrintError( "Failed to create actor list!\nPL: %s\n", plGetError());
	}
}

void Act_Shutdown( void ) {
	plDestroyLinkedList( actorList );
}
