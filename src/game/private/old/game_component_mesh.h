// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "game_private.h"

typedef struct GameMeshComponent {
	ApeMaterial *material;
	PLGMesh *mesh;

	ApeEntityComponent *transformComponent;
} GameMeshComponent;
#define GAME_MESH_COMPONENT( A ) ( ( GameMeshComponent * ) ( A ) )

const ApeEntityComponentCallbackTable *gameMeshComponentCallbackTable( void );
