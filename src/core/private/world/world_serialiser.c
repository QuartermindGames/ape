// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <yin/node.h>

#include "ape_private.h"
#include "world.h"

void ape_world_serialize_( const ApeWorld *world, NdBranch *root )
{
	nd_branch_push_back_uint32( root, "version", APE_WORLD_VERSION );

	if ( world->globalProperties != NULL )
	{
		nd_branch_push_back_branch( root, world->globalProperties );
	}
}
