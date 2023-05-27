// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "game_private.h"
#include "game_component_transform.h"

typedef struct GameComponentCamera
{
	OgeCamera *camera;
	bool             isActive;
	YNCoreEntityComponent *transform;
} GameComponentCamera;
#define GCCAMERA( SELF ) ENTITY_COMPONENT_CAST( ( SELF ), GameComponentCamera )

YN_CORE_ENTITY_COMPONENT_BEGIN_PROPERTIES()
YN_CORE_ENTITY_COMPONENT_PROPERTY( GameComponentCamera, isActive, "Indicates if the camera should be active or not.", CMN_DATATYPE_BOOL )
YN_CORE_ENTITY_COMPONENT_END_PROPERTIES()

static void Spawn( YNCoreEntityComponent *self )
{
	self->userData = PL_NEW( GameComponentCamera );

	const PLVector3 *position, *angles;

	GCCAMERA( self )->transform = YnCore_Entity_GetComponentByName( self->entity, "transform" );
	if ( GCCAMERA( self )->transform != NULL )
	{
		position = &ECTRANSFORM( GCCAMERA( self )->transform )->translation;
		angles   = &ECTRANSFORM( GCCAMERA( self )->transform )->angles;
	}
	else
	{
		position = &pl_vecOrigin3;
		angles   = &pl_vecOrigin3;
	}

	GCCAMERA( self )->camera = ogeCreateCamera( "dummy", position, angles );
}

static void Destroy( YNCoreEntityComponent *self )
{
	ogeDestroyCamera( GCCAMERA( self )->camera );

	PL_DELETE( GCCAMERA( self ) );
}

static void Tick( YNCoreEntityComponent *self )
{
	// if there's no transform component, try checking again...
	if ( GCCAMERA( self )->transform == NULL )
		GCCAMERA( self )->transform = YnCore_Entity_GetComponentByName( self->entity, "transform" );
	if ( GCCAMERA( self )->transform == NULL )
		return;

	ogeSetCameraPosition( GCCAMERA( self )->camera, &ECTRANSFORM( GCCAMERA( self )->transform )->translation );
	ogeSetCameraAngles( GCCAMERA( self )->camera, &ECTRANSFORM( GCCAMERA( self )->transform )->angles );

	if ( GCCAMERA( self )->isActive )
		ogeMakeCameraActive( GCCAMERA( self )->camera );
}

const YNCoreEntityComponentCallbackTable *Game_Component_Camera_GetCallbackTable( void )
{
	static YNCoreEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );
	callbackTable.spawnFunction   = Spawn;
	callbackTable.destroyFunction = Destroy;
	callbackTable.tickFunction    = Tick;

	YN_CORE_ENTITY_HOOK_PROPERTIES( callbackTable );

	return &callbackTable;
}
