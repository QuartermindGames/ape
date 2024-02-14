// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "ape_private.h"
#include "world.h"

void ape_world_serialize_( const ApeWorld *world, NdBranch *root )
{
	ndPushBackUI32( root, "version", APE_WORLD_VERSION );

	if ( world->globalProperties != NULL )
	{
		ndPushBackBranch( root, world->globalProperties );
	}
}
