// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "ape_private.h"
#include "world.h"

void apeSerializeWorld( const ApeWorld *world, NdBranch *root )
{
	ndPushBackI32( root, "version", APE_WORLD_VERSION );
	ndPushBackBranch( root, world->globalProperties );
}
