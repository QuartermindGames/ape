// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "world.h"

ApeWorldRoom *apeCreateWorldRoom( void )
{
	ApeWorldRoom *room = PL_NEW( ApeWorldRoom );
	room->detailRooms  = PlCreateVectorArray( 0 );
	room->faces        = PlCreateVectorArray( 0 );
	room->portals      = PlCreateVectorArray( 0 );
	room->colour       = PlCreateColour4B( rand() % 255, rand() % 255, rand() % 255, 255 );

	return room;
}

void apeDestroyWorldRoom( ApeWorldRoom *room )
{
	PlDestroyVectorArray( room->detailRooms );
	PlDestroyVectorArray( room->faces );
	PlDestroyVectorArray( room->portals );

	PlDestroyLinkedList( room->lights );

	PlgDestroyMesh( room->mesh );

	// clean-up batches
	for ( unsigned int i = 0; i < room->numBatches; ++i )
	{
		PL_DELETE( room->batches[ i ].subMeshes );
		PL_DELETE( room->batches[ i ].firstSubMeshes );
	}
	PL_DELETE( room->batches );

	PL_DELETE( room );
}

ApeWorldFace **apeGetWorldRoomFaces( ApeWorldRoom *room, unsigned int *numFaces )
{
	*numFaces = PlGetNumVectorArrayElements( room->faces );
	return ( ApeWorldFace ** ) PlGetVectorArrayData( room->faces );
}
