/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

#include <plmodel/plm.h>

#include "client/renderer/material.h"

#define MODEL_MAX_MATERIALS 64

typedef struct ModelUserData
{
	Material *   materials[ MODEL_MAX_MATERIALS ];
	unsigned int numMaterials;
	MemRefCnt    mem;
} ModelUserData;

PLMModel *MDL_CacheModel( const char *path );
