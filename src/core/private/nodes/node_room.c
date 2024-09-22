// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "../world/world.h"

ApeRoom *ape_room_create( ApeWorldNode *parent, const char *name )
{
	ApeRoom *room = PL_NEW( ApeRoom );
	ape_world_node_setup_( &room->base, parent, APE_WORLD_NODE_TYPE_ROOM, name, &pl_vecOrigin3, &pl_vecOrigin3 );

	room->detailRooms = PlCreateVectorArray( 0 );
	room->faces       = PlCreateVectorArray( 0 );
	room->portals     = PlCreateVectorArray( 0 );

	// assign the room a random colour so it can be identified per debugging
	room->colour = PL_COLOURF32RGB( PlUniform0To1Random(),
	                                PlUniform0To1Random(),
	                                PlUniform0To1Random() );

	return room;
}

void ape_room_destroy_( void *data )
{
	ApeRoom *self = ( ApeRoom * ) data;

	PlDestroyVectorArray( self->detailRooms );
	PlDestroyVectorArray( self->faces );
	PlDestroyVectorArray( self->portals );

	PlDestroyLinkedList( self->lights );

	PlgDestroyMesh( self->mesh );

	PL_DELETE( self );
}

void ape_room_set_ambience( ApeRoom *self, PLColourF32 ambience )
{
	self->ambientLight = ambience;
}

ApeWorldFace **ape_world_room_get_faces_( ApeRoom *self, unsigned int *numFaces )
{
	return ( ApeWorldFace ** ) PlGetVectorArrayDataEx( self->faces, numFaces );
}
