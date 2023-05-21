// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "world.h"
#include "renderer_visibility.h"

PLLinkedList *VIS_GetVisibleFaces( OgeCamera *camera, PLLinkedList *faces )
{
	PLLinkedList     *visibleFaces = PlCreateLinkedList();
	PLLinkedListNode *faceNode = PlGetFirstNode( faces );
	while ( faceNode != NULL )
	{
		OgeWorldFace *face = PlGetLinkedListNodeUserData( faceNode );
		if ( face->flags & WORLD_FACE_FLAG_SKIP )
		{
			faceNode = PlGetNextLinkedListNode( faceNode );
			continue;
		}

		PL_GET_CVAR( "r.cullMode", cullMode );

		// Check the face is actually visible
		if ( cullMode->i_value > 0 && !YnCore_World_IsFaceVisible( face, camera ) )
		{
			faceNode = PlGetNextLinkedListNode( faceNode );
			continue;
		}

		PlInsertLinkedListNode( visibleFaces, face );

		faceNode = PlGetNextLinkedListNode( faceNode );
	}

	return visibleFaces;
}

PLLinkedList *VIS_GetVisiblePortals( OgeCamera *camera, PLLinkedList *faces )
{
	PLLinkedList     *visiblePortals = PlCreateLinkedList();
	PLLinkedListNode *faceNode = PlGetFirstNode( faces );
	while ( faceNode != NULL )
	{
		OgeWorldFace *face = PlGetLinkedListNodeUserData( faceNode );
		if ( !YnCore_World_IsFacePortal( face ) )
		{
			faceNode = PlGetNextLinkedListNode( faceNode );
			continue;
		}

		PlInsertLinkedListNode( visiblePortals, face );

		faceNode = PlGetNextLinkedListNode( faceNode );
	}

	return visiblePortals;
}
