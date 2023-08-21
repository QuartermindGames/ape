// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <yin/node.h>

#include "ape_private.h"
#include "world.h"
#include "entity/entity.h"
#include "client/renderer/renderer.h"

/****************************************
 * PRIVATE
 ****************************************/

static ApeLight *DeserializeLight( NdBranch *root )
{
	ApeLight *light = PL_NEW( ApeLight );

	light->position = ndGetVector3( root, "position", &pl_vecOrigin3 );
	light->angles   = ndGetVector3( root, "angles", &pl_vecOrigin3 );

	light->colour = ndGetColourF32( root, "colour", &PL_COLOURF32_WHITE );

	light->radius = ndGetF32ByName( root, "radius", 0.0f );
	light->flags  = ndGetUInt( root, "flags", 0 );

	return light;
}

static void DeserializeLights( NdBranch *root, ApeWorld *out )
{
	NdBranch *lights = ndGetChildByName( root, "lights" );
	if ( lights == NULL )
		return;

	unsigned int numLights = ndGetNumOfChildren( lights );
	if ( numLights == 0 )
		return;

	out->lights = PlCreateVectorArray( numLights );

	NdBranch *child = ndGetFirstChild( lights );
	while ( child != NULL )
	{
		PlPushBackVectorArrayElement( out->lights, DeserializeLight( child ) );
		child = ndGetNextChild( child );
	}
}

static void DeserializeMaterials( NdBranch *root, ApeWorld *out )
{
	NdBranch *materials = ndGetChildByName( root, "materials" );
	if ( materials == NULL )
		return;

	unsigned int numMaterials = ndGetNumOfChildren( materials );
	if ( numMaterials == 0 )
		return;

	if ( ndGetType( materials ) != ND_PROPERTY_STRING )
	{
		PRINT_WARNING( "Unexpected branch type for materials!\n" );
		return;
	}

	out->materials = PlCreateVectorArray( numMaterials );

	NdBranch *child = ndGetFirstChild( materials );
	while ( child != NULL )
	{
		PLPath path;
		if ( ndGetStr( child, path, sizeof( path ) ) == ND_ERROR_SUCCESS )
			PlPushBackVectorArrayElement( out->materials, apeCacheMaterial( path, APE_CACHE_WORLD, true, false ) );
		else
			PRINT_WARNING( "Failed to fetch string from materials list: %s\n", ndGetErrorMessage() );

		child = ndGetNextChild( child );
	}
}

static ApeWorldRoom *DeserializeRoom( NdBranch *root )
{
	ApeWorldRoom *room = apeCreateWorldRoom();

	room->tag = ndGetInt( root, "tag", 0 );

	room->bounds.mins = ndGetVector3( root, "mins", &pl_vecOrigin3 );
	room->bounds.maxs = ndGetVector3( root, "maxs", &pl_vecOrigin3 );

	room->isDetail     = ndGetBoolByName( root, "isDetail", false );
	room->ambientLight = ndGetColourF32( root, "ambience", &PL_COLOURF32_BLACK );
	room->flags        = ndGetUInt( root, "flags", 0 );

	return room;
}

static void DeserializeRooms( NdBranch *root, ApeWorld *out )
{
	NdBranch *rooms = ndGetChildByName( root, "rooms" );
	if ( rooms == NULL )
		return;

	unsigned int numRooms = ndGetNumOfChildren( rooms );
	if ( numRooms == 0 )
		return;

	out->rooms = PlCreateVectorArray( numRooms );

	NdBranch *child = ndGetFirstChild( rooms );
	while ( child != NULL )
	{
		PlPushBackVectorArrayElement( out->rooms, DeserializeRoom( child ) );
		child = ndGetNextChild( child );
	}
}

static void DeserializePortals( NdBranch *root, ApeWorld *out )
{
	NdBranch *portals = ndGetChildByName( root, "portals" );
	if ( portals == NULL )
		return;
}

static void DeserializeGeometry( NdBranch *root, ApeWorld *out )
{
	DeserializeMaterials( root, out );
	DeserializeRooms( root, out );
	DeserializePortals( root, out );
}

/****************************************
 * PUBLIC
 ****************************************/

ApeWorld *apeDeserializeWorld( NdBranch *root, ApeWorld *out )
{
	unsigned int version = ndGetUInt( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 )
	{
		PRINT_WARNING( "Failed to find world version!\n" );
		return NULL;
	}
	else if ( version > APE_WORLD_VERSION )
	{
		PRINT_WARNING( "Unsupported world version! (%d > %d)\n", version, APE_WORLD_VERSION );
		return NULL;
	}

	NdBranch *propertyList = ndGetChildByName( root, "properties" );
	if ( propertyList != NULL )
	{
		// Copy the branch, so we can pass it over to the game logic later
		out->globalProperties = ndCopyBranch( propertyList );

		out->ambience    = ndGetColourF32( out->globalProperties, "ambience", &PL_COLOURF32( 0.5f, 0.5f, 0.5f, 1.0f ) );
		out->sunColour   = ndGetColourF32( out->globalProperties, "sunColour", &PL_COLOURF32_BLACK );
		out->sunPosition = ndGetVector3( out->globalProperties, "sunPosition", &pl_vecOrigin3 );

		out->clearColour = ndGetColourF32( out->globalProperties, "clearColour", &PL_COLOURF32_BLACK );

		out->fogColour = ndGetColourF32( out->globalProperties, "fogColour", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 0.0f ) );
		out->fogFar    = ndGetF32ByName( out->globalProperties, "fogFar", 11.0f );
		out->fogNear   = ndGetF32ByName( out->globalProperties, "fogNear", 32.0f );
	}

	DeserializeGeometry( root, out );
	DeserializeLights( root, out );

	return out;
}
