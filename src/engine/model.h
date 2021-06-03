/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include "renderer/material.h"

#define MODEL_MAX_MATERIALS 64

typedef struct Model
{
	struct PLModel *pModelData;

	Material *   materials[ MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;
} Model;
