// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct GameCameraEntityClass
{
	ApeCamera *camera;
	bool isActive;
	ApeEntityComponent *transform;
} GameCameraEntityClass;
#define GCCAMERA( SELF ) ENTITY_COMPONENT_CAST( ( SELF ), GameComponentCamera )
