/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "game_private.h"

typedef struct ECInputComponent
{
	unsigned int controllerSlot;
	ApeInputState inputStates[ APE_MAX_BUTTON_INPUTS ];
} ECInputComponent;

static void Spawn( ApeEntityComponent *self )
{
}
