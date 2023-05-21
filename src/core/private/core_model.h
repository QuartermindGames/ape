/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include <plmodel/plm.h>

#include "client/renderer/renderer_material.h"

#define MODEL_MAX_MATERIALS 64

typedef struct MDLUserData
{
	OgeMaterial *	 materials[ MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;
	OgeMemoryReference mem;
} MDLUserData;

PLMModel *Model_Cache( const char *path );
void      Model_Release( PLMModel *model );
