// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "game_private.h"
#include "game_component_transform.h"

typedef struct GameComponentCamera {
	ApeCamera *camera;
	bool isActive;
	ApeEntityComponent *transform;
} GameComponentCamera;
#define GCCAMERA( SELF ) ENTITY_COMPONENT_CAST( ( SELF ), GameComponentCamera )

APE_ENTITY_COMPONENT_BEGIN_PROPERTIES()
APE_ENTITY_COMPONENT_PROPERTY( GameComponentCamera, isActive, "Indicates if the camera should be active or not.", CMN_DATATYPE_BOOL )
APE_ENTITY_COMPONENT_END_PROPERTIES()

static void Spawn( ApeEntityComponent *self ) {
	self->userData = PL_NEW( GameComponentCamera );

	const PLVector3 *position, *angles;

	GCCAMERA( self )->transform = apeGetEntityComponentByName( self->entity, "transform" );
	if ( GCCAMERA( self )->transform != NULL ) {
		position = &ECTRANSFORM( GCCAMERA( self )->transform )->translation;
		angles = &ECTRANSFORM( GCCAMERA( self )->transform )->angles;
	} else {
		position = &pl_vecOrigin3;
		angles = &pl_vecOrigin3;
	}

	GCCAMERA( self )->camera = apeCreateCamera( "dummy", position, angles );
}

static void Destroy( ApeEntityComponent *self ) {
	apeDestroyCamera( GCCAMERA( self )->camera );

	PL_DELETE( GCCAMERA( self ) );
}

static void Tick( ApeEntityComponent *self ) {
	// if there's no transform component, try checking again...
	if ( GCCAMERA( self )->transform == NULL ) {
		GCCAMERA( self )->transform = apeGetEntityComponentByName( self->entity, "transform" );
	}
	if ( GCCAMERA( self )->transform == NULL ) {
		return;
	}

	apeSetCameraPosition( GCCAMERA( self )->camera, &ECTRANSFORM( GCCAMERA( self )->transform )->translation );
	apeSetCameraAngles( GCCAMERA( self )->camera, &ECTRANSFORM( GCCAMERA( self )->transform )->angles );

	if ( GCCAMERA( self )->isActive ) {
		apeMakeCameraActive( GCCAMERA( self )->camera );
	}
}

const ApeEntityComponentCallbackTable *Game_Component_Camera_GetCallbackTable( void ) {
	static ApeEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );
	callbackTable.spawnFunction = Spawn;
	callbackTable.destroyFunction = Destroy;
	callbackTable.tickFunction = Tick;

	APE_ENTITY_HOOK_PROPERTIES( callbackTable );

	return &callbackTable;
}
