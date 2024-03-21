// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "world.h"

ApeWorldRoom *ape_world_room_create( void )
{
	ApeWorldRoom *room = PL_NEW( ApeWorldRoom );

	ape_world_node_setup_header( &room->header, APE_WORLD_NODE_TYPE_ROOM );

	room->detailRooms = PlCreateVectorArray( 0 );
	room->faces = PlCreateVectorArray( 0 );
	room->portals = PlCreateVectorArray( 0 );

	// assign the room a random colour so it can be identified per debugging
	room->colour = PL_COLOURF32RGB( PlUniform0To1Random(),
	                                PlUniform0To1Random(),
	                                PlUniform0To1Random() );

	return room;
}

void ape_world_room_destroy( ApeWorldRoom *self )
{
	PlDestroyVectorArray( self->detailRooms );
	PlDestroyVectorArray( self->faces );
	PlDestroyVectorArray( self->portals );

	PlDestroyLinkedList( self->lights );

	PlgDestroyMesh( self->mesh );

	PL_DELETE( self );
}

ApeWorldFace **ape_world_room_get_faces_( ApeWorldRoom *self, unsigned int *numFaces )
{
	return ( ApeWorldFace ** ) PlGetVectorArrayDataEx( self->faces, numFaces );
}
