// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "world.h"

ApeRoom *ape_world_room_create( void )
{
	ApeRoom *room = PL_NEW( ApeRoom );
	room->detailRooms = PlCreateVectorArray( 0 );
	room->faces = PlCreateVectorArray( 0 );
	room->portals = PlCreateVectorArray( 0 );

	// assign the room a random colour so it can be identified per debugging
	room->colour = PL_COLOURF32RGB( PlUniform0To1Random(),
	                                PlUniform0To1Random(),
	                                PlUniform0To1Random() );

	return room;
}

void ape_world_room_destroy( ApeRoom *room )
{
	PlDestroyVectorArray( room->detailRooms );
	PlDestroyVectorArray( room->faces );
	PlDestroyVectorArray( room->portals );

	PlDestroyLinkedList( room->lights );

	PlgDestroyMesh( room->mesh );

	PL_DELETE( room );
}

ApeWorldFace **ape_world_room_get_faces_( ApeRoom *room, unsigned int *numFaces )
{
	return ( ApeWorldFace ** ) PlGetVectorArrayDataEx( room->faces, numFaces );
}

ApeRoom **ape_world_room_get_detail_rooms( ApeRoom *room, unsigned int *numDetailRooms )
{
	*numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
	return ( ApeRoom ** ) PlGetVectorArrayData( room->detailRooms );
}
