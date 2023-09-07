// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "game_private.h"
#include "game_class_camera.h"
#include "game_component_transform.h"

APE_ENTITY_COMPONENT_BEGIN_PROPERTIES()
APE_ENTITY_COMPONENT_PROPERTY( EntityClassCamera, isActive, "Indicates if the camera should be active or not.", COM_DATATYPE_BOOL )
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

const ApeEntityClassTable *gameGetCameraClassTable( void ) {
	static ApeEntityClassTable classTable;
	PL_ZERO_( classTable );
	classTable.Spawn = Spawn;
	classTable.Destroy = Destroy;
	classTable.Tick = Tick;

	APE_ENTITY_HOOK_PROPERTIES( callbackTable );

	return &callbackTable;
}
