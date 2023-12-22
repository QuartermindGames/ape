// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Cook utility

#pragma once

#include <plcore/pl.h>
#include <plmodel/plm.h>

#include "common.h"
#include "common_project.h"

#include "yin/node.h"

#define WARN( ... ) \
	printf( "WARNING: " __VA_ARGS__ );
#define ERROR( ... )           \
	{                          \
		printf( __VA_ARGS__ ); \
		exit( EXIT_FAILURE );  \
	}

typedef struct CookState
{
	const char *projectName;
} CookState;
extern CookState cook_state;

void cook_world_process( const char *worldName );
void cook_model_process( const char *modelName );
