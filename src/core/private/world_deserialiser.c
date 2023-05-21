// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "core_private.h"
#include "world.h"
#include "entity/entity.h"

static void DeserializeIdentifierTag( YNNodeBranch *node, char *dest )
{
	dest[ WORLD_PROP_TAG_LENGTH ] = '\0';
	const char *id                = YnNode_GetStringByName( node, "id", NULL );
	if ( id == NULL )
	{
		PlGenerateUniqueIdentifier( dest, WORLD_PROP_TAG_LENGTH - 1 );
		return;
	}

	strncpy( dest, id, WORLD_PROP_TAG_LENGTH - 1 );
}

static void DeserialiseSector( OgeWorld *world, YNNodeBranch *sectorNode, OgeWorldSector *sectorPtr )
{
	DeserializeIdentifierTag( sectorNode, sectorPtr->id );

	unsigned int numMeshes = PlGetNumVectorArrayElements( world->meshes );
	int          meshIndex = YnNode_GetI32ByName( sectorNode, "mesh", -1 );
	if ( meshIndex >= 0 && meshIndex < numMeshes )
	{
		sectorPtr->mesh = ( OgeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, meshIndex );
	}
	else
	{
		PRINT_WARNING( "Sector without valid body!\n" );
	}

	YnNode_DS_DeserializeVector3( YnNode_GetChildByName( sectorNode, "boundsMin" ), &sectorPtr->bounds.mins );
	YnNode_DS_DeserializeVector3( YnNode_GetChildByName( sectorNode, "boundsMax" ), &sectorPtr->bounds.maxs );

	YNNodeBranch *staticObjectList = YnNode_GetChildByName( sectorNode, "staticObjects" );
	if ( staticObjectList != NULL )
	{
		sectorPtr->numStaticObjects = YnNode_GetNumOfChildren( staticObjectList );
		sectorPtr->staticObjects    = PlCAlloc( sectorPtr->numStaticObjects, sizeof( OgeWorldObject ), true );
		YNNodeBranch *c                   = YnNode_GetFirstChild( staticObjectList );
		for ( unsigned int i = 0; i < sectorPtr->numStaticObjects; ++i )
		{
			if ( c == NULL )
			{
				PRINT_WARNING( "Hit an invalid object index: %d\n", i );
				sectorPtr->numStaticObjects = i;
				break;
			}

			meshIndex = YnNode_GetI32ByName( sectorNode, "mesh", -1 );
			if ( meshIndex >= 0 && meshIndex < numMeshes )
			{
				sectorPtr->staticObjects[ i ].mesh = ( OgeWorldMesh * ) PlGetVectorArrayElementAt( world->meshes, meshIndex );
			}
			else
			{
				PRINT_WARNING( "Invalid mesh index encountered for static object!\n" );
			}

			YnNode_DS_DeserializeVector3( YnNode_GetChildByName( c, "translation" ), &sectorPtr->staticObjects[ i ].transform.translation );
			YnNode_DS_DeserializeVector3( YnNode_GetChildByName( c, "scale" ), &sectorPtr->staticObjects[ i ].transform.scale );
			NL_DS_DeserializeVector4( YnNode_GetChildByName( c, "rotation" ), ( PLVector4 * ) &sectorPtr->staticObjects[ i ].transform.rotation );

			c = YnNode_GetNextChild( c );
		}
	}
}

static void DeserialiseEntities( OgeWorld *world, YNNodeBranch *root )
{
	if ( root == NULL )
	{
		PRINT( "No entities in world, skipping.\n" );
		return;
	}

	unsigned int entityNum = 0;
	YNNodeBranch *child     = YnNode_GetFirstChild( root );
	while ( child != NULL )
	{
		const char *templateName = YnNode_GetStringByName( child, "templateName", NULL );
		if ( templateName != NULL )
		{
			const YNCoreEntityPrefab *entityTemplate = YnCore_EntityManager_GetPrefabByName( templateName );
			if ( entityTemplate != NULL )
			{
				OgeWorldEntity *worldEntity    = PL_NEW( OgeWorldEntity );
				worldEntity->entityTemplate = entityTemplate;

				YNNodeBranch *properties = YnNode_GetChildByName( child, "properties" );
				if ( properties != NULL )
					worldEntity->properties = YnNode_CopyBranch( properties );

				PlInsertLinkedListNode( world->entities, worldEntity );
			}
			else
				PRINT_WARNING( "Failed to find entity template \"%s\"!\n", templateName );
		}
		else
			PRINT_WARNING( "No template name provided for entity %u!\n", entityNum );

		child = YnNode_GetNextChild( child );
		entityNum++;
	}
}

OgeWorld *YnCore_WorldDeserialiser_Begin( YNNodeBranch *root, OgeWorld *out )
{
	int version = YnNode_GetI32ByName( root, "version", -1 );
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

	YNNodeBranch *propertyList = YnNode_GetChildByName( root, "properties" );
	if ( propertyList != NULL )
	{
		out->globalProperties = YnNode_CopyBranch( propertyList );

		/* set some of the global defaults */
		YnCore_World_SetupGlobalDefaults( out );

		YnNode_DS_DeserializeColourF32( YnNode_GetChildByName( out->globalProperties, "ambience" ), &out->ambience );
		YnNode_DS_DeserializeColourF32( YnNode_GetChildByName( out->globalProperties, "sunColour" ), &out->sunColour );
		YnNode_DS_DeserializeVector3( YnNode_GetChildByName( out->globalProperties, "sunPosition" ), &out->sunPosition );
		YnNode_DS_DeserializeColourF32( YnNode_GetChildByName( out->globalProperties, "clearColour" ), &out->clearColour );

		YnNode_DS_DeserializeColourF32( YnNode_GetChildByName( out->globalProperties, "fogColour" ), &out->fogColour );
		out->fogFar  = YnNode_GetF32ByName( out->globalProperties, "fogFar", 11.0f );
		out->fogNear = YnNode_GetF32ByName( out->globalProperties, "fogNear", 32.0f );

		YNNodeBranch *childProperty = YnNode_GetChildByName( out->globalProperties, "skyMaterials" );
		if ( childProperty != NULL )
		{
			out->numSkyMaterials = YnNode_GetNumOfChildren( childProperty );
			if ( out->numSkyMaterials > OGE_MAX_SKY_LAYERS )
			{
				PRINT_WARNING( "Only a maximum of %d sky layers are supported!\n", OGE_MAX_SKY_LAYERS );
				out->numSkyMaterials = OGE_MAX_SKY_LAYERS;
			}

			unsigned int i          = 0;
			YNNodeBranch *childIndex = YnNode_GetFirstChild( childProperty );
			while ( childIndex != NULL )
			{
				char buf[ PL_SYSTEM_MAX_PATH ];
				YnNode_GetStr( childIndex, buf, sizeof( buf ) );
				out->skyMaterials[ i++ ] = YnCore_Material_Cache( buf, YN_CORE_CACHE_GROUP_WORLD, true, false );

				childIndex = YnNode_GetNextChild( childIndex );
			}
		}
	}

	DeserialiseEntities( out, YnNode_GetChildByName( root, "entities" ) );

	YNNodeBranch *meshList = YnNode_GetChildByName( root, "meshes" );
	if ( meshList != NULL )
	{
		unsigned int numEntries = YnNode_GetNumOfChildren( meshList );
		out->meshes             = PlCreateVectorArray( numEntries );
		YNNodeBranch *c               = YnNode_GetFirstChild( meshList );
		for ( unsigned int i = 0; i < numEntries; ++i )
		{
			if ( c == NULL )
			{
				PRINT_WARNING( "Hit an invalid mesh index: %d\n", i );
				break;
			}

			PLPath path;
			YnNode_GetStr( c, path, sizeof( path ) );

			OgeWorldMesh *mesh = YnCore_WorldMesh_Load( path );
			if ( mesh == NULL )
				continue;

			PlPushBackVectorArrayElement( out->meshes, mesh );
		}

		// Check if we need to downsize the meshes list...
		PlShrinkVectorArray( out->meshes );
	}

	YNNodeBranch *sectorList = YnNode_GetChildByName( root, "sectors" );
	if ( sectorList != NULL )
	{
		out->numSectors = YnNode_GetNumOfChildren( sectorList );
		out->sectors    = PlCAlloc( out->numSectors, sizeof( OgeWorldSector ), true );
		YNNodeBranch *c       = YnNode_GetFirstChild( sectorList );
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
