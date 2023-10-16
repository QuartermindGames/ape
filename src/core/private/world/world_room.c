// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "world.h"

ApeWorldRoom *acl_room_create( void )
{
	ApeWorldRoom *room = PL_NEW( ApeWorldRoom );
	room->detailRooms = PlCreateVectorArray( 0 );
	room->faces = PlCreateVectorArray( 0 );
	room->portals = PlCreateVectorArray( 0 );

	// assign the room a random colour so it can be identified per debugging
	room->colour = PL_COLOURF32RGB( PlUniform0To1Random(),
	                                PlUniform0To1Random(),
	                                PlUniform0To1Random() );

	return room;
}

void acl_room_destroy( ApeWorldRoom *room )
{
	PlDestroyVectorArray( room->detailRooms );
	PlDestroyVectorArray( room->faces );
	PlDestroyVectorArray( room->portals );

	PlDestroyLinkedList( room->lights );

	PlgDestroyMesh( room->mesh );

	PL_DELETE( room );
}

ApeWorldFace **acl_room_get_faces( ApeWorldRoom *room, unsigned int *numFaces )


{
	return ( ApeWorldFace ** ) PlGetVectorArrayDataEx( room->faces, numFaces );
}

ApeWorldRoom **acl_room_get_detail_rooms( ApeWorldRoom *room, unsigned int *numDetailRooms )
{
	*numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
	return ( ApeWorldRoom ** ) PlGetVectorArrayData( room->detailRooms );
}

ApeWorldRoom **acl_room_get_detail_rooms( ApeWorldRoom *room, unsigned int *numDetailRooms )
{
	*numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
	return ( ApeWorldRoom ** ) PlGetVectorArrayData( room->detailRooms );
}
