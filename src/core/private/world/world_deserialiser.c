// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "core_private.h"
#include "world.h"
#include "entity/entity.h"

static void DeserialiseEntities( OgeWorld *world, NdBranch *root )
{
	if ( root == NULL )
	{
		PRINT( "No entities in world, skipping.\n" );
		return;
	}

	unsigned int entityNum = 0;
	NdBranch *child     = ndGetFirstChild( root );
	while ( child != NULL )
	{
		const char *templateName = ndGetStringByName( child, "templateName", NULL );
		if ( templateName != NULL )
		{
			const YNCoreEntityPrefab *entityTemplate = YnCore_EntityManager_GetPrefabByName( templateName );
			if ( entityTemplate != NULL )
			{
				OgeWorldEntity *worldEntity    = PL_NEW( OgeWorldEntity );
				worldEntity->entityTemplate = entityTemplate;

				NdBranch *properties = ndGetChildByName( child, "properties" );
				if ( properties != NULL )
					worldEntity->properties = ndCopyBranch( properties );

				PlInsertLinkedListNode( world->entities, worldEntity );
			}
			else
				PRINT_WARNING( "Failed to find entity template \"%s\"!\n", templateName );
		}
		else
			PRINT_WARNING( "No template name provided for entity %u!\n", entityNum );

		child = ndGetNextChild( child );
		entityNum++;
	}
}

OgeWorld *YnCore_WorldDeserialiser_Begin( NdBranch *root, OgeWorld *out )
{
	int version = ndGetI32ByName( root, "version", -1 );
	if ( version == -1 )
	{
		PRINT_WARNING( "Failed to find world version!\n" );
		return NULL;
	}
	else if ( version > YN_CORE_WORLD_VERSION )
	{
		PRINT_WARNING( "Unsupported world version! (%d > %d)\n", version, YN_CORE_WORLD_VERSION );
		return NULL;
	}

	NdBranch *propertyList = ndGetChildByName( root, "properties" );
	if ( propertyList != NULL )
	{
		out->globalProperties = ndCopyBranch( propertyList );

		/* set some of the global defaults */

		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "ambience" ), &out->ambience );
		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "sunColour" ), &out->sunColour );
		ndDS_DeserializeVector3( ndGetChildByName( out->globalProperties, "sunPosition" ), &out->sunPosition );
		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "clearColour" ), &out->clearColour );

		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "fogColour" ), &out->fogColour );
		out->fogFar  = ndGetF32ByName( out->globalProperties, "fogFar", 11.0f );
		out->fogNear = ndGetF32ByName( out->globalProperties, "fogNear", 32.0f );
	}

	DeserialiseEntities( out, ndGetChildByName( root, "entities" ) );

	NdBranch *meshList = ndGetChildByName( root, "meshes" );
	if ( meshList != NULL )
	{
		unsigned int numEntries = ndGetNumOfChildren( meshList );
		out->meshes             = PlCreateVectorArray( numEntries );
		NdBranch *c               = ndGetFirstChild( meshList );
		for ( unsigned int i = 0; i < numEntries; ++i )
		{
			if ( c == NULL )
			{
				PRINT_WARNING( "Hit an invalid mesh index: %d\n", i );
				break;
			}

			PLPath path;
			ndGetStr( c, path, sizeof( path ) );

			OgeWorldMesh *mesh = YnCore_WorldMesh_Load( path );
			if ( mesh == NULL )
				continue;

			PlPushBackVectorArrayElement( out->meshes, mesh );
		}

		// Check if we need to downsize the meshes list...
		PlShrinkVectorArray( out->meshes );
	}

	return out;
}
