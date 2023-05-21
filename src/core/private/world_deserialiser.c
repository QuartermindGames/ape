// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "core_private.h"
#include "world.h"
#include "entity/entity.h"

static void DeserializeIdentifierTag( NdBranch *node, char *dest )
{
	dest[ WORLD_PROP_TAG_LENGTH ] = '\0';
	const char *id                = ndGetStringByName( node, "id", NULL );
	if ( id == NULL )
	{
		PlGenerateUniqueIdentifier( dest, WORLD_PROP_TAG_LENGTH - 1 );
		return;
	}

	strncpy( dest, id, WORLD_PROP_TAG_LENGTH - 1 );
}

static void DeserialiseSector( OgeWorld *world, NdBranch *sectorNode, OgeWorldSector *sectorPtr )
{
	DeserializeIdentifierTag( sectorNode, sectorPtr->id );

	unsigned int numMeshes = PlGetNumVectorArrayElements( world->meshes );
	int          meshIndex = ndGetI32ByName( sectorNode, "mesh", -1 );
	if ( meshIndex >= 0 && meshIndex < numMeshes )
	{
		sectorPtr->mesh = ( OgeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, meshIndex );
	}
	else
	{
		PRINT_WARNING( "Sector without valid body!\n" );
	}

	ndDS_DeserializeVector3( ndGetChildByName( sectorNode, "boundsMin" ), &sectorPtr->bounds.mins );
	ndDS_DeserializeVector3( ndGetChildByName( sectorNode, "boundsMax" ), &sectorPtr->bounds.maxs );

	NdBranch *staticObjectList = ndGetChildByName( sectorNode, "staticObjects" );
	if ( staticObjectList != NULL )
	{
		sectorPtr->numStaticObjects = ndGetNumOfChildren( staticObjectList );
		sectorPtr->staticObjects    = PlCAlloc( sectorPtr->numStaticObjects, sizeof( OgeWorldObject ), true );
		NdBranch *c                   = ndGetFirstChild( staticObjectList );
		for ( unsigned int i = 0; i < sectorPtr->numStaticObjects; ++i )
		{
			if ( c == NULL )
			{
				PRINT_WARNING( "Hit an invalid object index: %d\n", i );
				sectorPtr->numStaticObjects = i;
				break;
			}

			meshIndex = ndGetI32ByName( sectorNode, "mesh", -1 );
			if ( meshIndex >= 0 && meshIndex < numMeshes )
			{
				sectorPtr->staticObjects[ i ].mesh = ( OgeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, meshIndex );
			}
			else
			{
				PRINT_WARNING( "Invalid mesh index encountered for static object!\n" );
			}

			ndDS_DeserializeVector3( ndGetChildByName( c, "translation" ), &sectorPtr->staticObjects[ i ].transform.translation );
			ndDS_DeserializeVector3( ndGetChildByName( c, "scale" ), &sectorPtr->staticObjects[ i ].transform.scale );
			ndDS_DeserializeVector4( ndGetChildByName( c, "rotation" ), ( PLVector4 * ) &sectorPtr->staticObjects[ i ].transform.rotation );

			c = ndGetNextChild( c );
		}
	}
}

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
		YnCore_World_SetupGlobalDefaults( out );

		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "ambience" ), &out->ambience );
		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "sunColour" ), &out->sunColour );
		ndDS_DeserializeVector3( ndGetChildByName( out->globalProperties, "sunPosition" ), &out->sunPosition );
		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "clearColour" ), &out->clearColour );

		ndDS_DeserializeColourF32( ndGetChildByName( out->globalProperties, "fogColour" ), &out->fogColour );
		out->fogFar  = ndGetF32ByName( out->globalProperties, "fogFar", 11.0f );
		out->fogNear = ndGetF32ByName( out->globalProperties, "fogNear", 32.0f );

		NdBranch *childProperty = ndGetChildByName( out->globalProperties, "skyMaterials" );
		if ( childProperty != NULL )
		{
			out->numSkyMaterials = ndGetNumOfChildren( childProperty );
			if ( out->numSkyMaterials > OGE_MAX_SKY_LAYERS )
			{
				PRINT_WARNING( "Only a maximum of %d sky layers are supported!\n", OGE_MAX_SKY_LAYERS );
				out->numSkyMaterials = OGE_MAX_SKY_LAYERS;
			}

			unsigned int i          = 0;
			NdBranch *childIndex = ndGetFirstChild( childProperty );
			while ( childIndex != NULL )
			{
				char buf[ PL_SYSTEM_MAX_PATH ];
				ndGetStr( childIndex, buf, sizeof( buf ) );
				out->skyMaterials[ i++ ] = YnCore_Material_Cache( buf, YN_CORE_CACHE_GROUP_WORLD, true, false );

				childIndex = ndGetNextChild( childIndex );
			}
		}
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

	NdBranch *sectorList = ndGetChildByName( root, "sectors" );
	if ( sectorList != NULL )
	{
		out->numSectors = ndGetNumOfChildren( sectorList );
		out->sectors    = PlCAlloc( out->numSectors, sizeof( OgeWorldSector ), true );
		NdBranch *c       = ndGetFirstChild( sectorList );
		for ( unsigned int i = 0; i < out->numSectors; ++i )
		{
			if ( c == NULL )
			{
				PRINT_WARNING( "Hit an invalid sector Index: %d\n", i );
				out->numSectors = i;
				break;
			}

			DeserialiseSector( out, c, &out->sectors[ i ] );
		}
	}
	else
		PRINT_WARNING( "No sectors specified for world!\n" );

	return out;
}
