/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plcore/pl_linkedlist.h>

#include "common/node.h"

#include "yin.h"
#include "actor.h"
#include "map.h"

#include "client/renderer/renderer.h"

static void Act_DrawBasic( Actor *self, void *userData )
{
	PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT ] );

	R_DrawAxesPivot( Act_GetPosition( self ), PLVector3( 0, 0, 0 ) );
}

void Monster_Collide( struct Actor *self, struct Actor *other, float force )
{
	/* decide what direction to push out from */
	PLVector3 pushDir = PlSubtractVector3( Act_GetPosition( other ), Act_GetPosition( self ) );
	/* need to this based on distance from center */
	float length = PlVector3Length( pushDir ) / 10000.0f;
	pushDir = PlScaleVector3F( pushDir, length * force );
	Act_SetVelocity( other, &pushDir );
}

static ActorSetup actorDefault = {
		.id = "point.null",
		.Spawn = NULL,
		.Tick = NULL,
		.Draw = Act_DrawBasic,
		.Collide = NULL,
		.Destroy = NULL,
		.Serialize = NULL,
		.Deserialize = NULL,
};

extern const ActorSetup actorPlayerSetup;// actor_player.c

extern const ActorSetup sg_actorShip;			// sg_actors.c
extern const ActorSetup sg_actorAsteroidSetup;	// sg_actors.c
extern const ActorSetup sg_actorProjectileSetup;// sg_actors.c
extern const ActorSetup sg_actorAsteroidManagerSetup;
extern const ActorSetup sg_actorPropSetup;

const ActorSetup *actorSpawnSetup[ MAX_ACTOR_TYPES ] = {
		[ACTOR_NONE] = &actorDefault,
		[ACTOR_PLAYER] = &actorPlayerSetup,
		[ACTOR_LIGHT] = NULL,
		[ACTOR_TRIGGER_VOLUME] = NULL,
		// sg
		[ACTOR_SG_SHIP] = &sg_actorShip,
		[ACTOR_SG_ASTEROID] = &sg_actorAsteroidSetup,
		[ACTOR_SG_ASTEROID_MANAGER] = &sg_actorAsteroidManagerSetup,
		[ACTOR_SG_PROJECTILE] = &sg_actorProjectileSetup,
		[ACTOR_SG_PROP] = &sg_actorPropSetup,
};

static PLLinkedList *actorList;

Actor *Act_SpawnActor( ActorType type, NLNode *nodeTree )
{
	Actor *actor = globalSystem.MAlloc( sizeof( Actor ), true );
	actor->node = PlInsertLinkedListNode( actorList, actor );
	actor->setup = *actorSpawnSetup[ type ];
	actor->type = type;

	actor->geoColliders = PlCreateLinkedList();
	if ( actor->geoColliders == NULL )
		PrintError( "Failed to create colliders list!\nPL: %s\n", PlGetError() );

	/* give everything a set of basic bounds */
	actor->bounds.maxs = PLVector3( 16.0f, 16.0f, 16.0f );
	actor->bounds.mins = PLVector3( -16.0f, -16.0f, -16.0f );

	if ( actor->setup.Spawn != NULL )
		actor->setup.Spawn( actor );

	if ( nodeTree != NULL )
	{
		NLNode *node;
		if ( ( node = NL_GetChildByName( nodeTree, "tagName" ) ) != NULL )
		{
			NL_GetStr( node, actor->tagName, sizeof( actor->tagName ) );
		}
		if ( ( node = NL_GetChildByName( nodeTree, "position" ) ) != NULL )
		{
			NL_DS_DeserializeVector3( node, &actor->position );
		}
		if ( ( node = NL_GetChildByName( nodeTree, "angles" ) ) != NULL )
		{
			NL_DS_DeserializeVector3( node, &actor->angles );
		}

		if ( actor->setup.Deserialize != NULL )
			actor->setup.Deserialize( actor, nodeTree );
	}

	return actor;
}

Actor *Act_SpawnActorById( const char *id, NLNode *nodeTree )
{
	for ( unsigned int i = 0; i < MAX_ACTOR_TYPES; ++i )
	{
		if ( actorSpawnSetup[ i ] == NULL || strcmp( actorSpawnSetup[ i ]->id, id ) != 0 )
			continue;

		return Act_SpawnActor( i, nodeTree );
	}

	PrintWarn( "Failed to find actor by id: %s\n", id );
	return NULL;
}

Actor *Act_DestroyActor( Actor *self )
{
	if ( self->setup.Destroy != NULL )
		self->setup.Destroy( self, self->userData );

	PlDestroyLinkedList( self->geoColliders );
	PlSetLinkedListNodeUserData( self->node, NULL );

	globalSystem.Free( self );

	return NULL;
}

void Act_DestroyActors( void )
{
	Print( "Destroying actors\n" );

	PLLinkedListNode *node = PlGetFirstNode( actorList );
	while ( node != NULL )
	{
		PLLinkedListNode *next = PlGetNextLinkedListNode( node );
		Actor *actor = PlGetLinkedListNodeUserData( node );
		
		if(actor != NULL)
		{
			Act_DestroyActor( actor );
		}
		
		PlDestroyLinkedListNode(actorList, node);
		
		node = next;
	}
}

ActorType Act_GetType( const Actor *self ) { return self->type; }

void Act_SetPosition( Actor *self, const PLVector3 *position ) { self->position = *position; }
PLVector3 Act_GetPosition( const Actor *self ) { return self->position; }

void Act_SetVelocity( Actor *self, const PLVector3 *velocity ) { self->velocity = *velocity; }
PLVector3 Act_GetVelocity( const Actor *self ) { return self->velocity; }

void Act_SetAngle( Actor *self, float angle ) { self->angle = angle; }
float Act_GetAngle( const Actor *self ) { return self->angle; }

void Act_SetAngles( Actor *self, const PLVector3 *angles ) { self->angles = *angles; }
PLVector3 Act_GetAngles( const Actor *self ) { return self->angles; }

struct WorldSector *Act_GetWorldSector( Actor *self ) { return self->sector; }
void Act_SetWorldSector( Actor *self, struct WorldSector *sector ) { self->sector = sector; }

void Act_SetViewPitch( Actor *self, float viewPitch ) { self->viewPitch = viewPitch; }
float Act_GetViewPitch( const Actor *self ) { return self->viewPitch; }

void Act_SetViewOffset( Actor *self, float viewOffset ) { self->viewOffset = viewOffset; }
float Act_GetViewOffset( Actor *self ) { return self->viewOffset; }

void Act_SetUserData( Actor *self, void *userData ) { self->userData = userData; }
void *Act_GetUserData( Actor *self ) { return self->userData; }

void Act_SetCurrentFrame( Actor *self, unsigned int frame ) { self->currentFrame = frame; }
unsigned int Act_GetCurrentFrame( const Actor *self ) { return self->currentFrame; }

void Act_SetBounds( Actor *self, PLVector3 mins, PLVector3 maxs )
{
	if ( mins.x > maxs.x || mins.y > maxs.y || mins.z > maxs.z )
		PrintError( "Invalid bounds for actor (mins %s, maxs %s)!\n", PlPrintVector3( &mins, pl_int_var ), PlPrintVector3( &maxs, pl_int_var ) );

	self->bounds.maxs = maxs;
	self->bounds.mins = mins;
}

const PLCollisionAABB *Act_GetBounds( Actor *self ) { return &self->bounds; }

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
	return PlIsAabbIntersecting( &self->bounds, &other->bounds );
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

bool Act_IsVisible( Actor *self )
{
	Camera *camera = R_GetGlobalCamera();
	if ( camera == NULL )
		return false;

#if 1
	self->bounds.origin = self->position;
	return PlgIsBoxInsideView( camera->internal, &self->bounds );
#else
	return PlgIsSphereInsideView( camera->internal, &( PLCollisionSphere ){
															.origin = self->position,
															.radius = 128.0f } );
#endif
}

void Act_DrawActors( void )
{
	PROFILE_START( PROFILE_DRAW_ACTORS );

	PLLinkedListNode *curNode = PlGetFirstNode( actorList );
	while ( curNode != NULL )
	{
		Actor *actor = PlGetLinkedListNodeUserData( curNode );
		if ( actor != NULL && Act_IsVisible( actor ) )
		{
			if ( actor->setup.Draw )
				actor->setup.Draw( actor, actor->userData );
		}

#if 0
PLVector3 absOrigin = PlGetAabbAbsOrigin( &actor->bounds, actor->position );
		PlgSetShaderProgram( defaultShaderPrograms[ RS_SHADER_DEFAULT_VERTEX ] );

		PLColour boxColour;
		if ( Act_IsVisible( actor ) )
			boxColour = PL_COLOUR_GREEN;
		else
			boxColour = PL_COLOUR_RED;

		PlgDrawBoundingVolume( &actor->bounds, boxColour );
		PlgDrawBoundingVolume( &PlSetupCollisionAABB( absOrigin, PLVector3( -16.0f, -16.0f, -16.0f ), PLVector3( 16.0f, 16.0f, 16.0f ) ), PL_COLOUR_BLUE );

		PLLinkedListNode *colliderNode = PlGetFirstNode( actor->geoColliders );
		while ( colliderNode != NULL )
		{
			MapFace *face = PlGetLinkedListNodeUserData( colliderNode );

			PLCollisionPlane plane	   = PlSetupCollisionPlane( face->bounds.absOrigin, PlgGetPolygonFaceNormal( face->polygon ) );
			PLCollision		 collision = PlIsSphereIntersectingPlane( &PlSetupCollisionSphere( absOrigin, 16.0f ), &plane );
			if ( collision.penetration > 0.0f )
			{
				PlgDrawBoundingVolume( &face->bounds, PL_COLOUR_RED );

				R_DrawAxesPivot( collision.contactPoint, plane.normal );

				PLMatrix4 transform = PlMatrix4Identity();
				PlgDrawSimpleLine( transform, face->bounds.absOrigin, PlAddVector3( face->bounds.absOrigin, PlScaleVector3F( plane.normal, 64.0f ) ), PLColour( 255, 255, 0, 255 ) );
				PlgDrawSimpleLine( transform, actor->bounds.origin, collision.contactPoint, PLColour( 0, 255, 0, 255 ) );
			}
			else
				PlgDrawBoundingVolume( &face->bounds, PL_COLOUR_GREEN );

			colliderNode = PlGetNextLinkedListNode( colliderNode );
		}
#endif

		curNode = PlGetNextLinkedListNode( curNode );
	}

	PROFILE_END( PROFILE_DRAW_ACTORS );
}

#define GRAVITY 7.0f
void Act_TickActors( void *userData, double delta )
{
	u_unused( userData );
	u_unused( delta );

	PLLinkedListNode *index = PlGetFirstNode( actorList );
	while ( index != NULL )
	{
		PLLinkedListNode *next = PlGetNextLinkedListNode( index );
		Actor *actor = PlGetLinkedListNodeUserData( index );
		if ( actor == NULL )
		{
			PlDestroyLinkedListNode(actorList, index);
			index = next;
			
			continue;
		}

		PlAnglesAxes( actor->angles, NULL, NULL, &actor->forward );

		if ( actor->movementType == ACTOR_MOVEMENT_PHYSICS )
		{
#if 0
			static const float friction = 4.0f;
#else
			static const float friction = 10.0f;
#endif
			if ( actor->velocity.x != 0 )
				actor->velocity.x -= ( actor->velocity.x / ( friction - ( float ) delta ) );
			if ( actor->velocity.y != 0 )
				actor->velocity.y -= ( actor->velocity.y / ( friction - ( float ) delta ) );
			if ( actor->velocity.z != 0 )
				actor->velocity.z -= ( actor->velocity.z / ( friction - ( float ) delta ) );

#if 0
			actor->velocity.y = -GRAVITY;
#endif
		}

		actor->oldPosition = actor->position;
		actor->position = PlAddVector3( actor->position, actor->velocity );

		PLVector3 nPos = PlSubtractVector3( actor->position, actor->oldPosition );
		nPos = PlSubtractVector3( actor->position, nPos );

		/* check actor vs actor collision */
		if ( actor->setup.Collide != NULL )
		{
			/* ensure bounds origin is kept updated */
			actor->bounds.origin = nPos;

			Actor *collider = Act_CheckCollisions( actor );
			if ( collider != NULL && collider->setup.Collide != NULL )
				actor->setup.Collide( actor, collider, actor->userData );

			/* and now check actor vs world collision */

			PlDestroyLinkedListNodes( actor->geoColliders );

			/* first need to figure out what faces we're intersecting with */
#if 0// todo: revisit...
			unsigned int numFaces;
			MapFace *    faces = Map_GetFacesForSector( actor->area, &numFaces );
			for ( unsigned int i = 0; i < numFaces; ++i )
			{
				if ( !PlIsAabbIntersecting( &actor->bounds, &faces[ i ].bounds ) )
					continue;

				/* convert the face into a plane */
				PLCollisionPlane plane = PLCollisionPlane( faces[ i ].bounds.absOrigin, PlgGetPolygonFaceNormal( faces[ i ].polygon ) );

				/* now see if we're hitting anything */
				PLVector3         absOrigin = PlGetAabbAbsOrigin( &actor->bounds, nPos );
				PLCollisionSphere colSphere = PLCollisionSphere( absOrigin, 16.0f );
				PLCollision       collision = PlIsSphereIntersectingPlane( &colSphere, &plane );
				if ( collision.penetration > 0.0f )
				{
					/* printf( "penetration: %f\n", collision.penetration ); */
					actor->position = PlAddVector3( actor->position, PlScaleVector3F( PlNormalizeVector3( collision.contactNormal ), collision.penetration / GRAVITY ) );

					float d = PlRadiansToDegrees( PlVector3Length( PlNormalizeVector3( collision.contactNormal ) ) );

					PLLinkedListNode *node = PlInsertLinkedListNode( actor->geoColliders, &faces[ i ] );
					if ( node == NULL )
						PrintError( "Failed to insert node into colliders list!\n" );
				}
			}
#endif
		}

		if ( actor->setup.Tick != NULL )
		{
			actor->setup.Tick( actor, actor->userData );
		}

		index = next;
	}

	Sch_PushTask( "actor_tick", Act_TickActors, NULL, delta );
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

void Act_Initialize( void )
{
	Print( "Initializing actor manager\n" );

	actorList = PlCreateLinkedList();
	if ( actorList == NULL )
		PrintError( "Failed to create actor list!\nPL: %s\n", PlGetError() );
}

void Act_Shutdown( void )
{
	if ( actorList == NULL )
		return;

	PLLinkedListNode *node = PlGetFirstNode( actorList );
	while ( node != NULL )
	{
		PLLinkedListNode *next = PlGetNextLinkedListNode( node );
		
		Actor *actor = PlGetLinkedListNodeUserData( node );
		if(actor != NULL)
		{
			Act_DestroyActor( actor );
		}
		
		PlDestroyLinkedListNode(actorList, node);
		
		node = next;
	}

	PlDestroyLinkedList( actorList );
	actorList = NULL;
}
