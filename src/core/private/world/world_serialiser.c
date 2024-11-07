// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "acm/public/acm/acm.h"

#include "ape_private.h"
#include "world.h"

void ape_world_serialize_( const ApeWorld *world, AcmBranch *root )
{
	acm_push_ui32( root, "version", APE_WORLD_VERSION );

	if ( world->globalProperties != NULL )
	{
		acm_push_branch( root, world->globalProperties );
	}
}
