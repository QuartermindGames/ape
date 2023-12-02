// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "world.h"

SSAclWorldRoom *ss_acl_room_create( void )
{
	SSAclWorldRoom *room = PL_NEW( SSAclWorldRoom );
	room->detailRooms = PlCreateVectorArray( 0 );
	room->faces = PlCreateVectorArray( 0 );
	room->portals = PlCreateVectorArray( 0 );

	// assign the room a random colour so it can be identified per debugging
	room->colour = PL_COLOURF32RGB( PlUniform0To1Random(),
	                                PlUniform0To1Random(),
	                                PlUniform0To1Random() );

	return room;
}

void ss_acl_room_destroy( SSAclWorldRoom *room )
{
	PlDestroyVectorArray( room->detailRooms );
	PlDestroyVectorArray( room->faces );
	PlDestroyVectorArray( room->portals );

	PlDestroyLinkedList( room->lights );

	PlgDestroyMesh( room->mesh );

	PL_DELETE( room );
}

SSAclWorldFace **ss_acl_room_get_faces( SSAclWorldRoom *room, unsigned int *numFaces )
{
	return ( SSAclWorldFace ** ) PlGetVectorArrayDataEx( room->faces, numFaces );
}

SSAclWorldRoom **ss_acl_room_get_detail_rooms( SSAclWorldRoom *room, unsigned int *numDetailRooms )
{
	*numDetailRooms = PlGetNumVectorArrayElements( room->detailRooms );
	return ( SSAclWorldRoom ** ) PlGetVectorArrayData( room->detailRooms );
}
