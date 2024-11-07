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
	ApeRoom *room = ( ApeRoom * ) self;
	acm_push_string( root, "path", room->path, true );
	acm_push_ui32( root, "flags", room->flags );
	acm_push_array_f32( root, "colour", ( float * ) &room->colour, 4 );
	acm_push_array_f32( root, "ambience", ( float * ) &room->ambientLight, 4 );
	acm_push_ui32( root, "reverb", room->reverbPreset );

	return root;
}

ApeWorldNode *ape_room_deserialize_( ApeWorldNode *parent, AcmBranch *root )
{
	ApeRoom *self = ape_room_create( parent, "temp" );

	const char *path = acm_get_string( root, "path", nullptr );
	if ( path != nullptr )
	{
		snprintf( self->path, sizeof( self->path ), "%s", path );
	}

	self->flags        = ACM_GET_INT( self->flags, root, "flags", 0 );
	self->colour       = acm_get_colour_f32( root, "colour", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f ) );
	self->ambientLight = acm_get_colour_f32( root, "ambience", &PL_COLOURF32( 0.0f, 0.0f, 0.0f, 1.0f ) );
	self->reverbPreset = ACM_GET_INT( self->flags, root, "reverb", 0 );

	self->isDirty = true;

	return &self->base;
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

#if !defined( APE_NO_EDITOR )

bool ape_room_set_save_path( ApeRoom *self, const char *path )
{
	if ( PlSetupPath( self->savePath, false, "%s", path ) == nullptr )
	{
		ape_warning_( "Invalid path provided: %s\n", PlGetError() );
		return false;
	}

	return true;
}

const char *ape_room_get_save_path( const ApeRoom *self )
{
	if ( *self->savePath == '\0' )
	{
		return nullptr;
	}

	return self->savePath;
}

#endif
