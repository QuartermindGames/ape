// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_linkedlist.h>

#include <yin/node.h>

#include "ape_private.h"
#include "actor.h"
#include "world/world.h"

#include "client/renderer/renderer.h"

void Monster_Collide( struct Actor *self, struct Actor *other, float force )
{
	/* decide what direction to push out from */
	PLVector3 pushDir = PlSubtractVector3( Act_GetPosition( other ), Act_GetPosition( self ) );
	/* need to this based on distance from center */
	float length = PlVector3Length( pushDir );
	pushDir = PlScaleVector3F( pushDir, ( length / 10000.0f ) * force );
	other->velocity = PlAddVector3( other->velocity, pushDir );
	//Act_SetVelocity( other, &pushDir );
}

static ActorSetup actorDefault = {
        .id = "point.null",
        .Spawn = NULL,
        .Tick = NULL,
        .Collide = NULL,
        .Destroy = NULL,
        .Serialize = NULL,
        .Deserialize = NULL,
};

extern const ActorSetup actorPlayerSetup;// actor_player.c
extern const ActorSetup sg_actorPropSetup;

const ActorSetup *actorSpawnSetup[ MAX_ACTOR_TYPES ] = {
        [ACTOR_NONE] = &actorDefault,
        [ACTOR_PLAYER] = &actorPlayerSetup,
        [ACTOR_LIGHT] = NULL,
        [ACTOR_TRIGGER_VOLUME] = NULL,
        // sg
        [ACTOR_SG_PROP] = &sg_actorPropSetup,
};

static PLLinkedList *actorList;

Actor *Act_SpawnActor( ActorType type, NdBranch *nodeTree )
{
	Actor *actor = PL_NEW( Actor );
	actor->node = PlInsertLinkedListNode( actorList, actor );
	actor->setup = *actorSpawnSetup[ type ];
	actor->type = type;

	actor->geoColliders = PlCreateLinkedList();
	if ( actor->geoColliders == NULL )
	{
		PRINT_ERROR( "Failed to create colliders list!\nPL: %s\n", PlGetError() );
	}

	/* give everything a set of basic bounds */
	actor->collisionVolume.maxs = PLVector3( 16.0f, 16.0f, 16.0f );
	actor->collisionVolume.mins = PLVector3( -16.0f, -16.0f, -16.0f );
	actor->visibilityVolume = actor->collisionVolume;

	if ( actor->setup.Spawn != NULL )
		actor->setup.Spawn( actor );

	if ( nodeTree != NULL )
	{
		NdBranch *node;
		if ( ( node = nd_branch_get_child_by_name( nodeTree, "tagName" ) ) != NULL )
		{
			nd_branch_get_string( node, actor->tagName, sizeof( actor->tagName ) );
		}
		if ( ( node = nd_branch_get_child_by_name( nodeTree, "position" ) ) != NULL )
		{
			//nd_ds_deserialize_vector3( node, &actor->position );
		}
		if ( ( node = nd_branch_get_child_by_name( nodeTree, "angles" ) ) != NULL )
		{
			//nd_ds_deserialize_vector3( node, &actor->angles );
		}

		if ( actor->setup.Deserialize != NULL )
			actor->setup.Deserialize( actor, nodeTree );
	}

	return actor;
}

Actor *Act_SpawnActorById( const char *id, NdBranch *nodeTree )
{
	for ( unsigned int i = 0; i < MAX_ACTOR_TYPES; ++i )
	{
		if ( actorSpawnSetup[ i ] == NULL || strcmp( actorSpawnSetup[ i ]->id, id ) != 0 )
		{
			continue;
		}

		return Act_SpawnActor( i, nodeTree );
	}

	PRINT_WARNING( "Failed to find actor by id: %s\n", id );
	return NULL;
}

Actor *Act_DestroyActor( Actor *self )
{
	if ( self->setup.Destroy != NULL )
	{
		self->setup.Destroy( self, self->userData );
	}

	PlDestroyLinkedList( self->geoColliders );
	PlSetLinkedListNodeUserData( self->node, NULL );

	PlFree( self );

	return NULL;
}

ActorType Act_GetType( const Actor *self ) { return self->type; }

void Act_SetPosition( Actor *self, const PLVector3 *position ) { self->position = *position; }
PLVector3 Act_GetPosition( const Actor *self ) { return self->position; }

float Act_GetAngle( const Actor *self ) { return self->angle; }

void Act_SetWorldSector( Actor *self, struct ApeWorldRoom *sector ) { self->sector = sector; }

void Act_SetViewOffset( Actor *self, float viewOffset ) { self->viewOffset = viewOffset; }
float Act_GetViewOffset( Actor *self ) { return self->viewOffset; }

void Act_SetUserData( Actor *self, void *userData ) { self->userData = userData; }
void *Act_GetUserData( Actor *self ) { return self->userData; }

void Act_SetBounds( Actor *self, PLVector3 mins, PLVector3 maxs )
{
	if ( mins.x > maxs.x || mins.y > maxs.y || mins.z > maxs.z )
		PRINT_ERROR( "Invalid bounds for actor (mins %s, maxs %s)!\n", PlPrintVector3( &mins, PL_VAR_I32 ), PlPrintVector3( &maxs, PL_VAR_I32 ) );

	self->collisionVolume.maxs = maxs;
	self->collisionVolume.mins = mins;
}

void Act_SetVisibilityVolume( Actor *self, const PLVector3 *mins, const PLVector3 *maxs )
{
	if ( mins->x > maxs->x || mins->y > maxs->y || mins->z > maxs->z )
		PRINT_ERROR( "Invalid visibility volume for actor (mins %s, maxs %s)!\n", PlPrintVector3( mins, PL_VAR_I32 ), PlPrintVector3( maxs, PL_VAR_I32 ) );

	self->visibilityVolume.maxs = *maxs;
	self->visibilityVolume.mins = *mins;
}

PLVector3 Act_GetForward( const Actor *self )
{
	return self->forward;
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

bool Act_IsVisible( Actor *self, ApeCamera *camera )
{
	if ( camera == NULL )
		return false;

#if 1
	self->visibilityVolume.origin = self->position;
	return PlgIsBoxInsideView( camera->internal, &self->visibilityVolume );
#else
	return PlgIsSphereInsideView( camera->internal, &( PLCollisionSphere ){
	                                                        .origin = self->position,
	                                                        .radius = 128.0f } );
#endif
}

#if 0
void Act_DrawActors( ApeCamera *camera, ApeWorldRoom *sector )
{
	PLLinkedListNode *index = PlGetFirstNode( actorList );
	while ( index != NULL )
	{
		PLLinkedListNode *next = PlGetNextLinkedListNode( index );
		Actor *actor = PlGetLinkedListNodeUserData( index );
		if ( actor == NULL || actor->sector != sector )
		{
			index = next;
			continue;
		}

		if ( Act_IsVisible( actor, camera ) )
		{
			if ( actor->setup.Draw )
				actor->setup.Draw( actor, actor->userData );
		}

		PL_GET_CVAR( "r/showActorBounds", showActorBounds );
		if ( showActorBounds->b_value )
		{
			PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );

			PLColour boxColour;
			if ( Act_IsVisible( actor, camera ) )
			{
				boxColour = PL_COLOUR_GREEN;
			}
			else
			{
				boxColour = PL_COLOUR_RED;
			}

			PlgDrawBoundingVolume( &actor->visibilityVolume, &boxColour );
			PlgDrawBoundingVolume( &actor->collisionVolume, &PL_COLOUR_WHITE );

			PlgDrawBoundingVolume( &PL_COLLISION_AABB( actor->position, PLVector3( -8.0f, -8.0f, -8.0f ), PLVector3( 8.0f, 8.0f, 8.0f ) ), &PL_COLOUR_BLUE );

#	if 1
			PLLinkedListNode *colliderNode = PlGetFirstNode( actor->geoColliders );
			while ( colliderNode != NULL )
			{
				ApeWorldFace *face = PlGetLinkedListNodeUserData( colliderNode );

				PLCollisionPlane plane = PlSetupCollisionPlane( face->bounds.absOrigin, face->normal );
				PLCollision collision = PlIsSphereIntersectingPlane( &PlSetupCollisionSphere( actor->position, 16.0f ), &plane );
				if ( collision.penetration > 0.0f )
				{
					PlgDrawBoundingVolume( &face->bounds, &PL_COLOUR_RED );

					arl_draw_axis_pivot( collision.contactPoint, plane.normal, 1.0f );

					PLMatrix4 transform = PlMatrix4Identity();
					PlgDrawSimpleLine( transform, face->bounds.absOrigin, PlAddVector3( face->bounds.absOrigin, PlScaleVector3F( plane.normal, 64.0f ) ), PLColour( 255, 255, 0, 255 ) );
					PlgDrawSimpleLine( transform, actor->collisionVolume.origin, collision.contactPoint, PLColour( 0, 255, 0, 255 ) );
				}
				else
				{
					PlgDrawBoundingVolume( &face->bounds, &PL_COLOUR_GREEN );
				}

				colliderNode = PlGetNextLinkedListNode( colliderNode );
			}
#	endif
		}

		index = next;
	}
}
#endif

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

Actor *Act_GetByTag( const char *tag, Actor *start )
{
	PLLinkedListNode *node = ( start == NULL ) ? PlGetFirstNode( actorList ) : PlGetNextLinkedListNode( start->node );
	while ( node != NULL )
	{
		Actor *actor = PlGetLinkedListNodeUserData( node );
		if ( actor != NULL && strncmp( tag, actor->tagName, sizeof( actor->tagName ) ) == 0 )
			return actor;

		node = PlGetNextLinkedListNode( node );
	}

	return NULL;
}
