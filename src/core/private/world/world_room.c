// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "world.h"

ApeWorldRoom *apeCreateWorldRoom( void ) {
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

void apeDestroyWorldRoom( ApeWorldRoom *room ) {
	PlDestroyVectorArray( room->detailRooms );
	PlDestroyVectorArray( room->faces );
	PlDestroyVectorArray( room->portals );

	PlDestroyLinkedList( room->lights );

	PlgDestroyMesh( room->mesh );

	PL_DELETE( room );
}

ApeWorldFace **apeGetWorldRoomFaces( ApeWorldRoom *room, unsigned int *numFaces ) {
	*numFaces = PlGetNumVectorArrayElements( room->faces );
	return ( ApeWorldFace ** ) PlGetVectorArrayData( room->faces );
}
