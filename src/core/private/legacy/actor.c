// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"
#include "actor.h"
#include "world/world.h"

#include "renderer/renderer.h"

void Monster_Collide( Actor *self, Actor *other, float force )
{
	/* decide what direction to push out from */
	PLVector3 pushDir = PlSubtractVector3( Act_GetPosition( other ), Act_GetPosition( self ) );
	/* need to this based on distance from center */
	float length = PlVector3Length( pushDir );
	pushDir = PlScaleVector3F( pushDir, ( length / 10000.0f ) * force );
	other->velocity = PlAddVector3( other->velocity, pushDir );
	//Act_SetVelocity( other, &pushDir );
}

/****************************************
 * COLLISION
 ****************************************/

bool Act_IsColliding( Actor *self, Actor *other )
{
	// todo: we need to be smarter, what about cases where an actor is crossing
	//  the boundary?
	return PlIsAabbIntersecting( &self->collisionVolume, &other->collisionVolume );
}

Actor *Act_CheckCollisions( Actor *self )
{
	/* in the future, perhaps it's worth tracking multiple lists per sector? */
	PLLinkedListNode *curNode = PlGetFirstNode( actorList );
	while ( curNode != NULL )
	{
		Actor *actor = PlGetLinkedListNodeUserData( curNode );
		if ( actor == NULL )
		{
			/* Destroyed actor */
			curNode = PlGetNextLinkedListNode( curNode );
			continue;
		}

		/* "don't have time to play with myself" */
		if ( actor == self )
		{
			curNode = PlGetNextLinkedListNode( curNode );
			continue;
		}

		if ( Act_IsColliding( self, actor ) )
			return actor;

		curNode = PlGetNextLinkedListNode( curNode );
	}

	return NULL;
}

/****************************************
 * RENDERING
 ****************************************/

#define GRAVITY 7.0f
void Act_TickActors( void *userData, double delta )
{
	PLLinkedListNode *index = PlGetFirstNode( actorList );
	while ( index != NULL )
	{
		PLLinkedListNode *next = PlGetNextLinkedListNode( index );
		Actor *actor = PlGetLinkedListNodeUserData( index );
		if ( actor == NULL )
		{
			PlDestroyLinkedListNode( index );
			index = next;

			continue;
		}

		if ( actor->movementType == ACTOR_MOVEMENT_PHYSICS )
		{
			static const float friction = 4.0f;
			if ( actor->velocity.x != 0 )
			{
				actor->velocity.x -= ( actor->velocity.x / ( friction - ( float ) delta ) );
			}
			if ( actor->velocity.y != 0 )
			{
				actor->velocity.y -= ( actor->velocity.y / ( friction - ( float ) delta ) );
			}
			if ( actor->velocity.z != 0 )
			{
				actor->velocity.z -= ( actor->velocity.z / ( friction - ( float ) delta ) );
			}

#if 1
			actor->velocity.y = -GRAVITY;
#endif
		}

		actor->oldPosition = actor->position;
		actor->position = PlAddVector3( actor->position, actor->velocity );

		PLVector3 nPos = PlSubtractVector3( actor->position,
		                                    PlSubtractVector3( actor->position, actor->oldPosition ) );

		/* ensure bounds origin is kept updated */
		actor->collisionVolume.origin = nPos;

		/* check actor vs actor collision */
		if ( actor->setup.Collide != NULL )
		{
			Actor *collider = Act_CheckCollisions( actor );
			if ( collider != NULL && collider->setup.Collide != NULL )
			{
				actor->setup.Collide( actor, collider, actor->userData );

				if ( PlGetLinkedListNodeUserData( index ) == NULL )
				{
					/* Actor was destroyed by collision. */
					continue;
				}
			}
		}

		/* and now check actor vs world collision */

		/* first need to figure out what faces we're intersecting with */

#if 0
		PlDestroyLinkedListNodes( actor->geoColliders );
		if ( actor->sector != NULL )
		{
			unsigned int numFaces;
			ApeWorldFace **faces = YnCore_WorldSector_GetMeshFaces( actor->sector, &numFaces );
			for ( unsigned int i = 0; i < numFaces; ++i )
			{
				if ( !PlIsAabbIntersecting( &actor->collisionVolume, &faces[ i ]->bounds ) )
				{
					continue;
				}

				/* convert the face into a plane */
				PLCollisionPlane plane = PlSetupCollisionPlane( faces[ i ]->bounds.absOrigin, faces[ i ]->normal );

				/* now see if we're hitting anything */
				PLVector3         absOrigin = PlGetAabbAbsOrigin( &actor->collisionVolume, nPos );
				PLCollisionSphere colSphere = PlSetupCollisionSphere( absOrigin, 16.0f );
				PLCollision       collision = PlIsSphereIntersectingPlane( &colSphere, &plane );
				if ( collision.penetration > 0.0f )
				{
					//printf( "penetration: %f\n", collision.penetration );
					actor->position = PlAddVector3( actor->position, PlScaleVector3F( PlNormalizeVector3( collision.contactNormal ), collision.penetration / GRAVITY ) );

					float d = PL_RAD2DEG( PlVector3Length( PlNormalizeVector3( collision.contactNormal ) ) );

					PLLinkedListNode *node = PlInsertLinkedListNode( actor->geoColliders, &faces[ i ] );
					if ( node == NULL )
					{
						PRINT_ERROR( "Failed to insert node into colliders list!\n" );
					}
				}
			}
			PL_DELETE( faces );
		}
#endif

		if ( actor->setup.Tick != NULL )
		{
			actor->setup.Tick( actor, actor->userData );
		}

		index = next;
	}

	apePushScheduledTask( "actor_tick", Act_TickActors, NULL, delta );
}
