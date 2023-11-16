// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "ape_private.h"
#include "world.h"
#include "client/renderer/renderer.h"

/****************************************
 * PRIVATE
 ****************************************/

static const unsigned int MAX_VISIBILITY_DEPTH = 256;// we'll go through 256 portals maximum (maybe hook this to a var)

static PLVectorArray *visibleLights = NULL;
static PLVectorArray *visibleRooms = NULL;

static PLVector3 viewPos = { 0.0f, 0.0f, 0.0f };
static int compare_lights( const void *a, const void *b ) {
	SS_Arl_Light *lightA = *( SS_Arl_Light ** ) a;
	SS_Arl_Light *lightB = *( SS_Arl_Light ** ) b;

	float da = PlVector3Length( PlSubtractVector3( lightA->position, viewPos ) );
	float db = PlVector3Length( PlSubtractVector3( lightB->position, viewPos ) );

	return ( da > db ) ? 1 : -1;
}

static void sort_lights( const SS_Arl_Camera *camera ) {
	if ( !ape_config_.level.sortLights ) {
		return;
	}

	if ( visibleLights == NULL ) {
		return;
	}

	viewPos = ss_arl_camera_get_position( camera );

	SS_Arl_Light **lights = ( SS_Arl_Light ** ) PlGetVectorArrayData( visibleLights );
	unsigned int numLights = PlGetNumVectorArrayElements( visibleLights );

	qsort( lights, numLights, sizeof( SS_Arl_Light * ), compare_lights );
}

/**
 * Right now, there's a giant fuck-off list of lights the world provides
 * and no association between the worlds and rooms, so we need to iterate
 * over every single damn light.
 */
static void build_visible_light_list( ApeWorld *world, SS_Arl_Camera *camera ) {
	if ( world->lights == NULL ) {
		return;
	}

	// determine what lights are visible -
	// for now this operates over all the lights in the world, urgh...
	PlClearVectorArray( visibleLights );
	for ( unsigned int i = 0; i < PlGetNumVectorArrayElements( world->lights ); ++i ) {
		SS_Arl_Light *light = PlGetVectorArrayElementAt( world->lights, i );

		if ( !( light->flags & APE_LIGHT_FLAG_ENABLED ) ) {
			continue;
		}

		if ( light->type != APE_LIGHT_TYPE_SUN ) {
			float distance = PlVector3Length( PlSubtractVector3( light->position, ss_arl_camera_get_position( camera ) ) );
			if ( distance > ape_config_.renderer.maxLightDistance ) {
				continue;
			}

			PLCollisionSphere sphere = PlSetupCollisionSphere( light->position, light->radius );
			if ( !PlgIsSphereInsideView( camera->internal, &sphere ) ) {
				continue;
			}
		}

		PlPushBackVectorArrayElement( visibleLights, light );
	}

	sort_lights( camera );

	ape_rendererPerformance_.numLights = PlGetNumVectorArrayElements( visibleLights );
}

static void build_visible_room_list( ApeWorld *world, SS_Arl_Camera *camera ) {
	PlClearVectorArray( visibleRooms );

	if ( camera->room == NULL ) {
		return;
	}
}

/****************************************
 * PUBLIC
 ****************************************/

void apeInitializeWorldVisibilitySystem_( void ) {
	assert( visibleLights == NULL && visibleRooms == NULL );
	visibleLights = PlCreateVectorArray( APE_MAX_LIGHTS_PER_PASS );
	visibleRooms = PlCreateVectorArray( MAX_VISIBILITY_DEPTH );
}

void apeShutdownWorldVisibilitySystem_( void ) {
	PlDestroyVectorArray( visibleLights );
	visibleLights = NULL;
	PlDestroyVectorArray( visibleRooms );
	visibleRooms = NULL;
}

SS_Arl_Light **apeGetVisibleLights_( unsigned int *num ) {
	*num = PlGetNumVectorArrayElements( visibleLights );
	return ( SS_Arl_Light ** ) PlGetVectorArrayData( visibleLights );
}

ApeWorldRoom **apeGetVisibleRooms_( unsigned int *num ) {
	*num = PlGetNumVectorArrayElements( visibleRooms );
	return ( ApeWorldRoom ** ) PlGetVectorArrayData( visibleRooms );
}

void acl_level_build_visibility_lists_( void ) {
	ape_rendererPerformance_.numLights = 0;

	if ( ape_config_.level.skipDraw ) {
		return;
	}

	ApeWorld *world = acl_level_get_current();
	if ( world == NULL ) {
		return;
	}

	SS_Arl_Camera *camera = ss_arl_camera_get_active();
	if ( camera == NULL ) {
		return;
	}

	build_visible_light_list( world, camera );
	build_visible_room_list( world, camera );
}

void apeFlushWorldVisibilityLists_( void ) {
	PlClearVectorArray( visibleLights );
	PlClearVectorArray( visibleRooms );
}
