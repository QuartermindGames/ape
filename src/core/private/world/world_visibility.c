// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "world.h"
#include "client/renderer/renderer.h"

static const unsigned int MAX_VISIBILITY_DEPTH = 256;// we'll go through 256 portals maximum (maybe hook this to a var)

static PLVectorArray *visibleLights = NULL;
static PLVectorArray *visibleRooms  = NULL;

static PLVector3 viewPos = { 0.0f, 0.0f, 0.0f };
static int CompareLights( const void *a, const void *b )
{
	ApeLight *lightA = *( ApeLight ** ) a;
	ApeLight *lightB = *( ApeLight ** ) b;

	float da = PlVector3Length( PlSubtractVector3( lightA->position, viewPos ) );
	float db = PlVector3Length( PlSubtractVector3( lightB->position, viewPos ) );

	return ( da > db ) ? 1 : -1;
}

static void SortLights( const ApeCamera *camera )
{
	if ( !ape_config_.world.sortLights )
		return;

	if ( visibleLights == NULL )
		return;

	viewPos = apeGetCameraPosition( camera );

	ApeLight **lights      = ( ApeLight      **) PlGetVectorArrayData( visibleLights );
	unsigned int numLights = PlGetNumVectorArrayElements( visibleLights );

	qsort( lights, numLights, sizeof( ApeLight * ), CompareLights );
}

/**
 * Right now, there's a giant fuck-off list of lights the world provides
 * and no association between the worlds and rooms, so we need to iterate
 * over every single damn light.
 */
static void BuildVisibleLightList( ApeWorld *world, ApeCamera *camera )
{
	// determine what lights are visible -
	// for now this operates over all the lights in the world, urgh...
	PlClearVectorArray( visibleLights );
	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->lights ); ++i )
	{
		ApeLight *light = PlGetVectorArrayElementAt( world->lights, i );

		if ( !( light->flags & APE_LIGHT_FLAG_ENABLED ) )
			continue;

		float distance = PlVector3Length( PlSubtractVector3( light->position, apeGetCameraPosition( camera ) ) );
		if ( distance > 64.0f )
			continue;

		PLCollisionSphere sphere = PlSetupCollisionSphere( light->position, light->radius );
		if ( !PlgIsSphereInsideView( camera->internal, &sphere ) )
			continue;

		PlPushBackVectorArrayElement( visibleLights, light );
		break;
	}

	SortLights( camera );

	ape_rendererPerformance_.numLights = PlGetNumVectorArrayElements( visibleLights );
}

static void BuildVisibleRoomList( ApeWorld *world, ApeCamera *camera )
{
	PlClearVectorArray( visibleRooms );

	if ( camera->room == NULL )
		return;
}

/////////////////////////////////////////////////////////////////

void apeInitializeWorldVisibilitySystem_( void )
{
	assert( visibleLights == NULL && visibleRooms == NULL );
	visibleLights = PlCreateVectorArray( APE_MAX_LIGHTS_PER_PASS );
	visibleRooms  = PlCreateVectorArray( MAX_VISIBILITY_DEPTH );
}

void apeShutdownWorldVisibilitySystem_( void )
{
	PlDestroyVectorArray( visibleLights );
	visibleLights = NULL;
	PlDestroyVectorArray( visibleRooms );
	visibleRooms = NULL;
}

ApeLight **apeGetVisibleLights_( unsigned int *num )
{
	*num = PlGetNumVectorArrayElements( visibleLights );
	return ( ApeLight ** ) PlGetVectorArrayData( visibleLights );
}

ApeWorldRoom **apeGetVisibleRooms_( void )
{
	return ( ApeWorldRoom ** ) PlGetVectorArrayData( visibleRooms );
}

void apeBuildWorldVisibiltyLists_( void )
{
	ape_rendererPerformance_.numLights = 0;

	PL_GET_CVAR( "world/draw", drawWorld );
	if ( drawWorld != NULL && !drawWorld->b_value )
		return;

	ApeWorld *world = apeGetCurrentWorld();
	if ( world == NULL )
		return;

	ApeCamera *camera = apeGetActiveCamera();
	if ( camera == NULL )
		return;

	BuildVisibleLightList( world, camera );
	BuildVisibleRoomList( world, camera );
}
