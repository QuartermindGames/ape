// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Deserialization methods specific to Volition's RFL format.

#include "ape_private.h"

#include "client/renderer/renderer.h"
#include "world.h"

#include "yin/core_fs.h"
#include "ape/ape_format_rfl.h"

//#define FLIP_WORLD// X coord needs to be flipped to match APE Tech coordinates...
#ifdef FLIP_WORLD
#	define FLIP_VECTOR( X ) ( ( X ).x *= -1 )
#else
#	define FLIP_VECTOR( X )
#endif

// gross versioning crap... this should probably be simplified

/*
 * RFL version history...
 * ------------------------------------
 * 180 - rf pc public demo
 * 200 - rf pc retail
 * 212 - lightmap support removed
 * 272 - rf2 ps2 public demo
 * 482 - punisher ps2 retail
 */

static void parse_static_geometry_textures( ApeWorld *world, PLFile *file )
{
	// fetch all the textures we'll be using
	uint32_t numTextures = PL_READUINT32( file, false, NULL );
	world->materials = PlCreateVectorArray( numTextures );
	for ( uint32_t i = 0; i < numTextures; ++i )
	{
		uint16_t size;
		char *textureName = ss_acl_fs_parse_string( file, &size );
		assert( textureName != NULL );
		if ( textureName == NULL )
		{
			PRINT_WARNING( "Invalid texture (%u) name! (%lu)\n", i, PlGetFileOffset( file ) );
			continue;
		}
		PRINT_DEBUG( "Texture: %s\n", textureName );

		char *c = strrchr( textureName, '.' );
		if ( c != NULL )
			*c = '\0';

		PLPath path;
		PlSetupPath( path, true, "materials/world/%s.mat.n", textureName );

		PlPushBackVectorArrayElement( world->materials, ss_arl_material_cache( path, APE_CACHE_WORLD, true, false ) );

		PL_DELETE( textureName );
	}
}

static ApeAudioReverbPreset get_eax_effect_id( const char *effectName )
{
	for ( unsigned int i = 0; i < APE_NUM_AUDIO_EFFECT_TYPES; ++i )
	{
		if ( pl_strcasecmp( effectName, APE_AUDIO_EFFECT_TYPES[ i ].name ) != 0 )
			continue;

		return APE_AUDIO_EFFECT_TYPES[ i ].effect;
	}

	PRINT_WARNING( "Unknown EAX effect name (%s)!\n", effectName );
	return APE_AUDIO_REVERB_PRESET_NONE;
}

static void parse_static_geometry_rooms( ApeWorld *world, PLFile *file, unsigned int version )
{
	// fetch and populate the room list
	unsigned int numRooms = PL_READUINT32( file, false, NULL );
	world->rooms = PlCreateVectorArray( numRooms );
	for ( unsigned int i = 0; i < numRooms; ++i )
	{
		ApeWorldRoom *room = acl_room_create();

		room->uid = ss_acl_fs_parse_int_ex( file, version, RFL_VERSION_RF1_PROTO, RFL_VERSION_MAX, -1 );

		room->bounds.mins = ss_acl_fs_parse_vector( file );
		FLIP_VECTOR( room->bounds.mins );
		room->bounds.maxs = ss_acl_fs_parse_vector( file );
		FLIP_VECTOR( room->bounds.maxs );

		room->flags = ss_acl_fs_parse_int_ex( file, version, RFL_VERSION_RF2_DEMO, RFL_VERSION_MAX, 0 );
		if ( version < RFL_VERSION_RF2_DEMO )
		{
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_SKY; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_COLD; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_OUTSIDE; }
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_AIRLOCK; }
			room->containsLiquid = ( bool ) PL_READUINT8( file, NULL );
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_AMBIENT; }
			room->isDetail = ( bool ) PL_READUINT8( file, NULL );
			if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_ALPHA; }
		}

		room->life = ss_acl_fs_parse_float( file );

		// Urgh, Volition removed this and then brought it back...
		if ( ( version >= RFL_VERSION_RF1_DEMO && version < RFL_VERSION_RF2_DEMO ) || version >= RFL_VERSION_RF2_RETAIL )
		{
			uint16_t size;
			char *eaxEffect = ss_acl_fs_parse_string( file, &size );
			if ( eaxEffect != NULL )
			{
				assert( isalpha( *eaxEffect ) );
				if ( isalpha( *eaxEffect ) )
					room->reverbPreset = get_eax_effect_id( eaxEffect );
				else
					PRINT_WARNING( "EAX effect is not a valid string! (%lu)", PlGetFileOffset( file ) );

				PL_DELETE( eaxEffect );
			}
		}

		if ( version >= RFL_VERSION_RF2_DEMO )
		{
			acl_fs_parse_float_ex( file, version, RFL_VERSION_RF2_RETAIL, RFL_VERSION_MAX, 0.0f );
			acl_fs_parse_float_ex( file, version, RFL_VERSION_RF2_RETAIL, RFL_VERSION_MAX, 0.0f );
			acl_fs_parse_float_ex( file, version, RFL_VERSION_RF2_RETAIL, RFL_VERSION_MAX, 0.0f );

			PLColour colour = ss_acl_fs_parse_colour( file );
			room->liquid.colour = PlColourU8ToF32( &colour );

			room->liquid.visibility = ss_acl_fs_parse_float( file );

			room->liquid.type = PlReadInt32( file, false, NULL );

			if ( version < RFL_VERSION_RF2_RETAIL )
			{
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = ss_acl_fs_parse_float( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
			}

			room->liquid.panU = ss_acl_fs_parse_float( file );
			room->liquid.panV = ss_acl_fs_parse_float( file );

			acl_fs_parse_float_ex( file, version, RFL_VERSION_RF2_RETAIL, RFL_VERSION_MAX, 0.0f );
			acl_fs_parse_float_ex( file, version, RFL_VERSION_RF2_RETAIL, RFL_VERSION_MAX, 0.0f );

			if ( version < RFL_VERSION_RF2_RETAIL )
			{
				ss_acl_fs_parse_colour( file );
				PlReadInt32( file, false, NULL );

				if ( room->flags & APE_WORLD_ROOM_FLAG_UNKNOWN0 )
				{
					uint16_t size;
					char *tmp = ss_acl_fs_parse_string( file, &size );
					PL_DELETE( tmp );
				}
			}
		}
		else
		{
			if ( room->containsLiquid )
			{
				room->liquid.depth = PlReadFloat32( file, false, NULL );
				assert( !isnan( room->liquid.depth ) );

				PLColour colour = ss_acl_fs_parse_colour( file );
				room->liquid.colour = PlColourU8ToF32( &colour );

				uint16_t size;
				char *liquidTextureName = ss_acl_fs_parse_string( file, &size );
				assert( liquidTextureName != NULL && *liquidTextureName != '\0' );
				PL_DELETE( liquidTextureName );

				room->liquid.visibility = ss_acl_fs_parse_float( file );

				room->liquid.type = PlReadInt32( file, false, NULL );
				room->liquid.alpha = PlReadInt32( file, false, NULL );
				if ( ( bool ) PL_READUINT8( file, NULL ) ) { room->flags |= APE_WORLD_ROOM_FLAG_PLANKTON; }
				room->liquid.ppmU = PlReadInt32( file, false, NULL );
				room->liquid.ppmV = PlReadInt32( file, false, NULL );

				room->liquid.angle = ss_acl_fs_parse_float( file );

				room->liquid.waveform = PlReadInt32( file, false, NULL );
				room->liquid.panU = ss_acl_fs_parse_float( file );
				room->liquid.panV = ss_acl_fs_parse_float( file );
			}

			if ( room->flags & APE_WORLD_ROOM_FLAG_AMBIENT )
			{
				PLColour colour = ss_acl_fs_parse_colour( file );
				room->ambientLight = PlColourU8ToF32( &colour );
			}
		}

		PlPushBackVectorArrayElement( world->rooms, room );
	}
}

static void parse_static_geometry_detail_rooms( ApeWorld *world, PLFile *file )
{
	// something about sorting rooms into detail rooms list???
	uint32_t numRooms = PL_READUINT32( file, false, NULL );
	assert( numRooms == PlGetNumVectorArrayElements( world->rooms ) );
	for ( uint32_t i = 0; i < numRooms; ++i )
	{
		uint32_t roomIndex = PL_READUINT32( file, false, NULL );
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );

		uint32_t numDetailRooms = PL_READUINT32( file, false, NULL );
		if ( room != NULL && room->detailRooms == NULL )
		{
			room->detailRooms = PlCreateVectorArray( numDetailRooms );
		}

		for ( uint32_t j = 0; j < numDetailRooms; ++j )
		{
			uint32_t detailRoomIndex = PL_READUINT32( file, false, NULL );
			ApeWorldRoom *detailRoom = PlGetVectorArrayElementAt( world->rooms, detailRoomIndex );
			assert( detailRoom != NULL );
			if ( room != NULL && detailRoom != NULL )
			{
				detailRoom->isDetail = true;
				PlPushBackVectorArrayElement( room->detailRooms, detailRoom );
			}
		}
	}
}

static void parse_static_geometry_portals( ApeWorld *world, PLFile *file )
{
	uint32_t numPortals = PL_READUINT32( file, false, NULL );
	world->portals = PlCreateVectorArray( numPortals );
	for ( uint32_t i = 0; i < numPortals; ++i )
	{
		uint32_t roomAIndex = PL_READUINT32( file, false, NULL );
		uint32_t roomBIndex = PL_READUINT32( file, false, NULL );

		PLVector3 mins = ss_acl_fs_parse_vector( file );
		FLIP_VECTOR( mins );
		PLVector3 maxs = ss_acl_fs_parse_vector( file );
		FLIP_VECTOR( maxs );

		ApeWorldRoom *roomA = PlGetVectorArrayElementAt( world->rooms, roomAIndex );
		ApeWorldRoom *roomB = PlGetVectorArrayElementAt( world->rooms, roomBIndex );
		assert( roomA != NULL && roomB != NULL );
		if ( roomA == NULL || roomB == NULL )
		{
			PRINT_WARNING( "Invalid rooms for portals!\n" );
			continue;
		}

		ApeWorldPortal *portal = PL_NEW( ApeWorldPortal );
		portal->roomA = roomA;
		portal->roomB = roomB;
		portal->mins = mins;
		portal->maxs = maxs;

		PlPushBackVectorArrayElement( roomA->portals, portal );
		PlPushBackVectorArrayElement( roomB->portals, portal );
		PlPushBackVectorArrayElement( world->portals, portal );
	}
}

static void parse_static_geometry_vertices( ApeWorld *world, PLFile *file )
{
	uint32_t numVertices = PL_READUINT32( file, false, NULL );
	world->vertices = PlCreateVectorArray( numVertices );
	for ( uint32_t i = 0; i < numVertices; ++i )
	{
		ApeWorldVertex *vertex = PL_NEW( ApeWorldVertex );
		vertex->position = ss_acl_fs_parse_vector( file );
		FLIP_VECTOR( vertex->position );
		PlPushBackVectorArrayElement( world->vertices, vertex );
	}
}

void ape_level_face_generate_bounds( ApeWorldFace *face )
{
	unsigned int numVertices = PlGetNumVectorArrayElements( face->vertices );
	ApeWorldFaceVertex **vertices = ( ApeWorldFaceVertex ** ) PlGetVectorArrayData( face->vertices );
	if ( numVertices == 0 )
		return;

	PLVector3 *boundVertices = PL_NEW_( PLVector3, numVertices );
	for ( unsigned int i = 0; i < numVertices; ++i )
		boundVertices[ i ] = vertices[ i ]->u->position;

	face->bounds = PlGenerateAabbFromCoords( boundVertices, numVertices, true );
	face->origin = PlGetAabbAbsOrigin( &face->bounds, pl_vecOrigin3 );

	PL_DELETE( boundVertices );
}

static void calculate_face_normal( ApeWorldFace *face )
{
	unsigned int numVertices = PlGetNumVectorArrayElements( face->vertices );
	assert( numVertices > 0 );
	if ( numVertices == 0 )
	{
		return;
	}

	//TODO
	assert( 0 );
}

static void parse_static_geometry_faces( ApeWorld *world, PLFile *file, unsigned int version )
{
	unsigned int numFaces = PL_READUINT32( file, false, NULL );
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		ApeWorldFace *face = PL_NEW( ApeWorldFace );

		face->edgeLoop = PlCreateLinkedList();

		// plane
		face->normal = acl_fs_parse_vector_ex( file, version, RFL_VERSION_RF1_DEMO, RFL_VERSION_MAX, &pl_vecOrigin3 );// normal
		FLIP_VECTOR( face->normal );
		face->offset = acl_fs_parse_float_ex( file, version, RFL_VERSION_RF1_DEMO, RFL_VERSION_MAX, 0.0f );// offset


		face->materialIndex = ss_acl_fs_parse_int( file );
		if ( face->materialIndex >= 0 )
		{
			face->material = PlGetVectorArrayElementAt( world->materials, face->materialIndex );
			assert( face->material != NULL );
			if ( face->material == NULL )
			{
				PRINT_WARNING( "Invalid material index (%d) (%lu)!\n", face->materialIndex, ( PlGetFileOffset( file ) - 4 ) );
				face->material = arl_material_get_default( APE_MATERIAL_DEFAULT_FALLBACK );
			}
		}
		// some texture indices are negative, which is valid...
		// we just don't handle it yet
		if ( face->material == NULL )
			face->material = ss_arl_get_fallback_material();

		int lightmapIndex = ss_acl_fs_parse_int_ex( file, version, RFL_VERSION_MIN, 211, -1 );
		ss_acl_fs_parse_int_ex( file, version, 266, RFL_VERSION_MAX, 0 );// unused?

		// ???
		ss_acl_fs_parse_int_ex( file, version, 49, RFL_VERSION_MAX, 0 );
		ss_acl_fs_parse_int_ex( file, version, 66, 211, 0 );
		ss_acl_fs_parse_int_ex( file, version, 66, 211, 0 );

		int32_t portalIndex = ss_acl_fs_parse_int( file );
		if ( portalIndex >= 0 )
		{
			face->portal = PlGetVectorArrayElementAt( world->portals, portalIndex );
			//assert( face->portal != NULL );
		}

		face->flags = PL_READUINT32( file, false, NULL );

		face->smoothingGroup = ss_acl_fs_parse_int( file );

		int32_t roomIndex = ss_acl_fs_parse_int( file );
		assert( roomIndex >= 0 );
		ApeWorldRoom *room = PlGetVectorArrayElementAt( world->rooms, roomIndex );
		assert( room != NULL );

		unsigned int numFaceVertices = PL_READUINT32( file, false, NULL );
		face->vertices = PlCreateVectorArray( numFaceVertices );
		for ( unsigned int j = 0; j < numFaceVertices; ++j )
		{
			ApeWorldFaceVertex *faceVertex = PL_NEW( ApeWorldFaceVertex );

			int32_t worldVertexIndex = PlReadInt32( file, false, NULL );
			assert( worldVertexIndex >= 0 );
			if ( worldVertexIndex >= 0 )
			{
				faceVertex->u = ( ApeWorldVertex * ) PlGetVectorArrayElementAt( world->vertices, worldVertexIndex );
				assert( faceVertex->u != NULL );
				if ( faceVertex->u == NULL )
					PRINT_ERROR( "Invalid world vertex for face!\n" );

				if ( faceVertex->u->adjacentFaces == NULL )
					faceVertex->u->adjacentFaces = PlCreateVectorArray( 1 );

				PlPushBackVectorArrayElement( faceVertex->u->adjacentFaces, face );
			}

			faceVertex->uv.x = ss_acl_fs_parse_float( file );
			assert( faceVertex->uv.x * faceVertex->uv.x >= 0.0f );
			faceVertex->uv.y = ss_acl_fs_parse_float( file );
			assert( faceVertex->uv.y * faceVertex->uv.y >= 0.0f );

			// Initially, we can just derive the face vertex normal from the face normal,
			// but these will need to be generated based on smoothing groups later
			faceVertex->normal = face->normal;

			if ( lightmapIndex >= 0 )
			{
				faceVertex->lightmapU = acl_fs_parse_float_ex( file, version, RFL_VERSION_MIN, 211, 0.0f );
				faceVertex->lightmapV = acl_fs_parse_float_ex( file, version, RFL_VERSION_MIN, 211, 0.0f );
			}

			faceVertex->colour.r = ss_acl_fs_parse_byte_ex( file, version, 212, RFL_VERSION_MAX, 0 );
			faceVertex->colour.g = ss_acl_fs_parse_byte_ex( file, version, 212, RFL_VERSION_MAX, 0 );
			faceVertex->colour.b = ss_acl_fs_parse_byte_ex( file, version, 212, RFL_VERSION_MAX, 0 );
			faceVertex->colour.a = ss_acl_fs_parse_byte_ex( file, version, 212, RFL_VERSION_MAX, 0 );

#if defined( FLIP_WORLD )
			PlInsertFrontLinkedListNode( face->edgeLoop, faceVertex );
#else
			PlInsertLinkedListNode( face->edgeLoop, faceVertex );
#endif

			PlPushBackVectorArrayElement( face->vertices, faceVertex );
		}

		// Older versions didn't store the face normal / offset,
		// so we'll need to calculate that here
		if ( version < 167 )
			calculate_face_normal( face );

		ape_level_face_generate_bounds( face );

		PlPushBackVectorArrayElement( room->faces, face );
	}
}

static void parse_static_geometry_lightmaps( ApeWorld *world, PLFile *file, unsigned int version )
{
	unsigned int numLightmaps = ( unsigned int ) ss_acl_fs_parse_int_ex( file, version, RFL_VERSION_MIN, 211, 0 );
	for ( unsigned int i = 0; i < numLightmaps; ++i )
	{
		PlReadInt32( file, false, NULL );// lightmap index
		PL_READUINT8( file, NULL );      // x start
		PL_READUINT8( file, NULL );      // y start

		uint8_t width = PL_READUINT8( file, NULL );// width
		assert( width != 0 );
		uint8_t height = PL_READUINT8( file, NULL );// height
		assert( height != 0 );

		float xPerMeter = ss_acl_fs_parse_float( file );// x pixels per meter
		float yPerMeter = ss_acl_fs_parse_float( file );// y pixels per meter

		PLVector3 min = ss_acl_fs_parse_vector( file );// min
		PLVector3 max = ss_acl_fs_parse_vector( file );// max

		ss_acl_fs_parse_vector( file );     // eq
		ss_acl_fs_parse_float( file );      // offset
		PlReadInt32( file, false, NULL );// should smooth
		PlReadInt32( file, false, NULL );// fullbright
		PlReadInt32( file, false, NULL );// dropped coefficient
		PlReadInt32( file, false, NULL );// u coefficient
		PlReadInt32( file, false, NULL );// v coefficient
		ss_acl_fs_parse_float( file );      // uv add x
		ss_acl_fs_parse_float( file );      // uv add y
		ss_acl_fs_parse_float( file );      // uv scale x
		ss_acl_fs_parse_float( file );      // uv scale y

		int32_t roomIndex = PlReadInt32( file, false, NULL );// room index
		assert( PlGetVectorArrayElementAt( world->rooms, roomIndex ) != NULL );
	}
}

static void parse_static_geometry_texture_movers( ApeWorld *world, PLFile *file )
{
	// texture movers
	uint32_t numTextureMovers = PL_READUINT32( file, false, NULL );
	for ( uint32_t i = 0; i < numTextureMovers; ++i )
	{
		int32_t faceIndex = PlReadInt32( file, false, NULL );
		assert( faceIndex >= 0 );

		ApeWorldFace *face;
		if ( faceIndex >= 0 )
		{
			//face = ( ApeWorldFace * ) PlGetVectorArrayElementAt( world->faces, faceIndex );
			//assert( face != NULL );
		}

		float uPanSpeed = ss_acl_fs_parse_float( file );
		float vPanSpeed = ss_acl_fs_parse_float( file );
	}
}

static ApeWorld *parse_static_geometry_chunk( ApeWorld *level, PLFile *file, unsigned int version )
{
	ss_acl_fs_parse_int_ex( file, version, RFL_VERSION_RF1_RETAIL_1_2, RFL_VERSION_MAX, 0 );// "modifiability" - unused

	// unused string
	uint16_t size;
	char *buf = ss_acl_fs_parse_string( file, &size );
	PL_DELETE( buf );

	ss_acl_fs_parse_int_ex( file, version, RFL_VERSION_MIN, 199, 0 );// "modifiability" - unused

	parse_static_geometry_textures( level, file );

	unsigned int numScrollingFaces = ss_acl_fs_parse_int_ex( file, version, RFL_VERSION_RF1_DEMO, RFL_VERSION_RF1_RETAIL_1_2, 0 );
	for ( unsigned int i = 0; i < numScrollingFaces; ++i )
	{
		assert( 0 );
		// todo

		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );
		ss_acl_fs_parse_float( file );//x
		ss_acl_fs_parse_float( file );//y
		ss_acl_fs_parse_float( file );//x
		ss_acl_fs_parse_float( file );//y
		ss_acl_fs_parse_float( file );//x
		ss_acl_fs_parse_float( file );//y
		ss_acl_fs_parse_float( file );//x
		ss_acl_fs_parse_float( file );//y
		PlReadInt8( file, NULL );
	}

	parse_static_geometry_rooms( level, file, version );
	parse_static_geometry_detail_rooms( level, file );

	// some currently unknown room-related data
	// (https://github.com/Marisa-Chan/RF2_rfl_rfc/blob/master/rfl_read.py#L264)
	unsigned int num = ss_acl_fs_parse_int_ex( file, version, 216, RFL_VERSION_MAX, 0 );
	for ( unsigned int i = 0; i < num; ++i )
	{
		ss_acl_fs_parse_int( file );
		ss_acl_fs_parse_int( file );
	}

	parse_static_geometry_portals( level, file );
	parse_static_geometry_vertices( level, file );
	parse_static_geometry_faces( level, file, version );
	parse_static_geometry_lightmaps( level, file, version );
	//TODO: commented out for now, as it's causing issues with newer levels -
	// 		and I get the impression it was removed but too lazy to check,
	// 		plus we're not doing anything with it right now anyway...
	//parse_static_geometry_texture_movers( level, file );

	return NULL;
}

static void parse_lights_chunk( ApeWorld *level, PLFile *file, unsigned int version )
{
	int32_t numLights = PlReadInt32( file, false, NULL );
	PlResizeVectorArray( level->lights, numLights );
	for ( int32_t i = 0; i < numLights; ++i )
	{
		SS_Arl_Light *light = PL_NEW( SS_Arl_Light );

		PlReadInt32( file, false, NULL );// id

		uint16_t size;
		char *tmp = ss_acl_fs_parse_string( file, &size );// class name
		PL_DELETE( tmp );

		light->position = ss_acl_fs_parse_vector( file );
		FLIP_VECTOR( light->position );

		ss_acl_fs_parse_mat3( file );// rotation

		tmp = ss_acl_fs_parse_string( file, &size );// script name
		PL_DELETE( tmp );

		light->isHidden = PL_READUINT8( file, NULL );     // hidden in editor
		light->flags = PL_READUINT32( file, false, NULL );// flags

		PLColour colour = ss_acl_fs_parse_colour( file );
		light->colour = PlColourU8ToF32( &colour );

		light->radius = ss_acl_fs_parse_float( file );// * 2.0f;

		ss_acl_fs_parse_float( file );// fov
		ss_acl_fs_parse_float( file );// fov dropoff
		ss_acl_fs_parse_float( file );// intensity at max range
		ss_acl_fs_parse_int( file );  // dropoff type
		ss_acl_fs_parse_float( file );// tube light width
		ss_acl_fs_parse_float( file );// on intensity
		ss_acl_fs_parse_float( file );// on time
		ss_acl_fs_parse_float( file );// on time variation
		ss_acl_fs_parse_float( file );// off intensity
		ss_acl_fs_parse_float( file );// off time
		ss_acl_fs_parse_float( file );// off time variation

		PlPushBackVectorArrayElement( level->lights, light );
	}
}

static void parse_player_start( ApeWorld *world, PLFile *file, unsigned int version )
{
	world->startPosition = ss_acl_fs_parse_vector( file );
	FLIP_VECTOR( world->startPosition );
	world->startOrientation = ss_acl_fs_parse_mat3( file );
}

static void parse_level_properties( ApeWorld *world, PLFile *file, unsigned int version )
{
	uint16_t size;
	char *texture = ss_acl_fs_parse_string( file, &size );
	if ( texture != NULL )
		PL_DELETE( texture );

	int hardness = PlReadInt32( file, false, NULL );

	PLColour ambience = ss_acl_fs_parse_colour( file );
	world->ambience = PlColourU8ToF32( &ambience );
	bool directionalAmbience = PL_READUINT8( file, NULL );

	PLColour fogColour = ss_acl_fs_parse_colour( file );
	world->fogColour = PlColourU8ToF32( &fogColour );
	world->fogNear = ss_acl_fs_parse_float( file );
	world->fogFar = ss_acl_fs_parse_float( file );

	// Ensure clear copies the fog, to ensure some level of consistency
	world->clearColour = world->fogColour;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeWorld *acl_level_load_file( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to load world: %s\n", PlGetError() );
		return NULL;
	}

	ApeWorld *level = acl_level_deserialize_rfl_( file );

	PlCloseFile( file );

	return level;
}

ApeWorld *acl_level_deserialize_rfl_( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	if ( magic != RFL_MAGIC )
	{
		PRINT_WARNING( "Invalid magic for world! (%x != %x)\n", magic, RFL_MAGIC );
		return NULL;
	}

	unsigned int version = PL_READUINT32( file, false, NULL );
	if ( version < RFL_VERSION_MIN || version > RFL_VERSION_MAX )
	{
		PRINT_WARNING( "Invalid version for world! (%d < %d || %d > %d)\n",
		               version, RFL_VERSION_MIN,
		               version, RFL_VERSION_MAX );
		return NULL;
	}

	uint32_t timestamp = PL_READUINT32( file, false, NULL );

	uint32_t objectOffset = PL_READUINT32( file, false, NULL );
	uint32_t editorOffset = PL_READUINT32( file, false, NULL );

	ApeWorld *level = acl_level_create();

	// read in all the chunks
	uint32_t numChunks = PL_READUINT32( file, false, NULL );
	uint32_t totalChunkSize = PL_READUINT32( file, false, NULL );

	uint16_t size;
	level->name = ss_acl_fs_parse_string_ex( file, &size, version, RFL_VERSION_RF1_DEMO, RFL_VERSION_MAX );
	if ( level->name != NULL )
		PRINT( "RFL level name: %s\n", level->name );

	char *modName = ss_acl_fs_parse_string_ex( file, &size, version, RFL_VERSION_RF1_DEMO, RFL_VERSION_RF1_RETAIL_1_2 );
	if ( modName != NULL )
	{
		PRINT( "RFL mod name: %s\n", modName );
		PL_DELETE( modName );
	}

	for ( uint32_t i = 0; i < numChunks; ++i )
	{
		uint32_t chunkId = PL_READUINT32( file, false, NULL );
		uint32_t chunkSize = PL_READUINT32( file, false, NULL );

		PLFileOffset offset = PlGetFileOffset( file );
		PLFileOffset nextChunk = offset + chunkSize;

		switch ( chunkId )
		{
			case RFL_CHUNK_GEOMETRY:
				parse_static_geometry_chunk( level, file, version );
				break;
			case RFL_CHUNK_LIGHTS:
				parse_lights_chunk( level, file, version );
				break;
			case RFL_CHUNK_PLAYER_START:
				parse_player_start( level, file, version );
				break;
			case RFL_CHUNK_LEVEL_PROPERTIES:
				parse_level_properties( level, file, version );
				break;
			default:
				PRINT_WARNING( "Skipping unknown chunk (%x : %u)\n", chunkId, offset );
				break;
		}

		// always do this afterwards, just on the off-chance the chunk failed to read
		PlFileSeek( file, nextChunk, PL_SEEK_SET );
	}

	return level;
}
