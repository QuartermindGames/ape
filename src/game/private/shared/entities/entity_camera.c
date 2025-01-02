// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: <purpose>
// Author:  <name>

#include "../game_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct CameraEntity
{
	ApeCamera *camera;
} CameraEntity;
#define CAMERA( SELF ) APE_ENT_CLASS( SELF, CameraEntity )

/////////////////////////////////////////////////////////////////////////////////////
// Public
