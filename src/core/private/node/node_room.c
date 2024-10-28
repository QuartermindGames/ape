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

void ape_room_destroy_( void *data, ApeWorldNode *parent )
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

PLColourF32 ape_room_get_ambience( ApeRoom *self )
{
	return self->ambientLight;
}

void ape_room_set_reverb_preset( ApeRoom *self, ApeAudioReverbPreset reverbPreset )
{
	self->reverbPreset = reverbPreset;
}

ApeAudioReverbPreset ape_room_get_reverb_preset( ApeRoom *self )
{
	return self->reverbPreset;
}

//TODO: deprecate!!! we use brushes now :(
ApeWorldFace **ape_world_room_get_faces_( ApeRoom *self, unsigned int *numFaces )
{
	return ( ApeWorldFace ** ) PlGetVectorArrayDataEx( self->faces, numFaces );
}

AcmBranch *ape_room_serialize_( void *self, AcmBranch *root )
{
	ApeRoom   *room       = ( ApeRoom         *) self;
	AcmBranch *roomBranch = acm_branch_push_back_object( root, "room" );
	acm_branch_push_back_string( roomBranch, "path", room->path, true );
	acm_branch_push_back_uint32( roomBranch, "flags", room->flags );
	acm_branch_push_back_float32_array( roomBranch, "colour", ( float * ) &room->colour, 4 );
	acm_branch_push_back_float32_array( roomBranch, "ambience", ( float * ) &room->ambientLight, 4 );
	acm_branch_push_back_uint32( roomBranch, "reverb", room->reverbPreset );

	return root;
}

bool ape_room_set_path( ApeRoom *self, const char *path )
{
	if ( PlSetupPath( self->path, false, "%s", path ) == nullptr )
	{
		ape_warning_( "Invalid path provided: %s\n", PlGetError() );
		return false;
	}

	return true;
}

const char *ape_room_get_path( const ApeRoom *self )
{
	return self->path;
}
