// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "world.h"

static const unsigned int MAX_VISIBILITY_DEPTH = 256;// we'll go through 256 portals maximum (maybe hook this to a var)

static PLVectorArray *apeDetermineVisibleRooms_( ApeWorld *world, ApeWorldRoom *startRoom, ApeCamera *camera )
{
}
