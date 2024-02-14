// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

typedef struct GameCameraEntityClass
{
	ApeCamera *camera;
	bool isActive;
	ApeEntityComponent *transform;
} GameCameraEntityClass;
#define GCCAMERA( SELF ) ENTITY_COMPONENT_CAST( ( SELF ), GameComponentCamera )
