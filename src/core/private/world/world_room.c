// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Specific logic for managing rooms, otherwise known as "sectors", within a world

#include "world.h"

OgeWorldRoom *ogeCreateWorldRoom( void )
{
	OgeWorldRoom *room = PL_NEW( OgeWorldRoom );
	room->detailRooms  = PlCreateVectorArray( 0 );
	room->faces        = PlCreateVectorArray( 0 );
	room->portals      = PlCreateVectorArray( 0 );
	return room;
}

void ogeDestroyWorldRoom( OgeWorldRoom *room )
{
	PlDestroyVectorArray( room->detailRooms );
	PlDestroyVectorArray( room->faces );
	PlDestroyVectorArray( room->portals );

	PL_DELETE( room );
}
