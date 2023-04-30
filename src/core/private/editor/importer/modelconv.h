// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>
#include <plmodel/plm.h>

#include "common.h"
#include "node.h"

#define Error( ... )                    \
	{                                   \
		fprintf( stderr, __VA_ARGS__ ); \
		abort();                        \
	}
