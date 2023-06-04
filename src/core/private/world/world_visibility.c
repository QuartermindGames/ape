// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "world.h"

static const unsigned int MAX_VISIBILITY_DEPTH = 256;// we'll go through 256 portals maximum (maybe hook this to a var)

static PLVectorArray *ogeDetermineVisibleRooms( ApeWorld *world, ApeWorldRoom *startRoom, ApeCamera *camera )
{
}
