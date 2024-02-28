// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "game_private.h"

typedef struct GameMeshComponent
{
	ApeMaterial *material;
	PLGMesh *mesh;

	ApeEntityComponent *transformComponent;
} GameMeshComponent;
#define GAME_MESH_COMPONENT( A ) ( ( GameMeshComponent * ) ( A ) )

const ApeEntityComponentCallbackTable *gameMeshComponentCallbackTable( void );
