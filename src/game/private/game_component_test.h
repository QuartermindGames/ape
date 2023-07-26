// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "game_private.h"

typedef struct GameTestComponent
{
	ApeEntityComponent *transformComponent;
	ApeEntityComponent *meshComponent;
} GameTestComponent;
#define GAME_TEST_COMPONENT( A ) ( ( GameTestComponent * ) ( A ) )

const ApeEntityComponentCallbackTable *gameTestComponentCallbackTable( void );
