// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "common.h"

bool comMountProject( const char *name );
void comUnmountProject( void );

const char *comGetProjectLocalPath( void );
