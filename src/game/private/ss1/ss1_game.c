// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ss1_game.h"

#include "menu/menu.h"

#include "../shared/integrations/integrations.h"
#include "../shared/physics/physics.h"
#include "../shared/game_team.h"
#include "../shared/components/component_movement.h"

#include "entities/entity_player.h"

SS1GameState ss1_gameState;

#define SS1_CONFIG "game_ss1"

const SS1Profession ss1_professions[ SS1_MAX_PROFESSIONS ] = {
        [SS1_PROFESSION_SHAMAN] = {
                                   .name            = "Shaman",
                                   .description     = "Healer",
                                   .maxHealth       = 100,
                                   .maxForwardSpeed = 10.0f,
                                   .maxStrafeSpeed  = 10.0f,
                                   },
        [SS1_PROFESSION_MACHINIST] = {
                                   .name            = "Machinist",
                                   .description     = "Engineer",
                                   .maxHealth       = 100,
                                   .maxForwardSpeed = 10.0f,
                                   .maxStrafeSpeed  = 10.0f,
                                   },
        [SS1_PROFESSION_TRICKSTER] = {
                                   .name            = "Trickster",
                                   .description     = "Spy",
                                   .maxHealth       = 100,
                                   .maxForwardSpeed = 10.0f,
                                   .maxStrafeSpeed  = 10.0f,
                                   },
        [SS1_PROFESSION_POUNDER] = {
                                   .name            = "Pounder",
                                   .description     = "Heavy",
                                   .maxHealth       = 200,
                                   .maxForwardSpeed = 10.0f,
                                   .maxStrafeSpeed  = 10.0f,
                                   },
};

static unsigned int teamResourcePools[ SS1_MAX_TEAMS ][ SS1_MAX_RESOURCE_TYPES ];

extern ApeEntityClassDefinition ss1_airshipEntityClass;
extern ApeEntityClassDefinition ss1_pawnEntityClass;
extern ApeEntityClassDefinition ss1_playerEntityClass;

static void toggle_camera( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	ss1_gameState.oldCameraState = ss1_gameState.cameraState;
	ss1_gameState.cameraState    = ( ss1_gameState.cameraState == SS1_CAMERA_STATE_THIRD_PERSON ) ? SS1_CAMERA_STATE_FREE : SS1_CAMERA_STATE_THIRD_PERSON;
}

static bool ss1_initialize()
{
	static constexpr int64_t DISCORD_CLIENT_ID = 822170320169074719;
	game_integrations_discord_initialize_( DISCORD_CLIENT_ID );
	game_integrations_discord_update_activity_( G_STR_( "Idling" ), nullptr, "qm1-logo", SS1_GAME_TITLE );

	game_register_standard_entity_components_();

	ape_register_entity_class( &ss1_airshipEntityClass );
	ape_register_entity_class( &ss1_pawnEntityClass );
	ape_register_entity_class( &ss1_playerEntityClass );

	ape_client_input_register_action( "ss1_toggle_camera", ( ApeInputButton[] ) { INPUT_BACK }, 1, ( ApeInputKey[] ) { 'z' }, 1, toggle_camera );

	PL_ZERO_( ss1_gameState );

#if !defined( NDEBUG )
	// validate all the professions are setup correctly
	for ( unsigned int i = 0; i < SS1_MAX_PROFESSIONS; ++i )
	{
		assert( ss1_professions[ i ].name != nullptr && ss1_professions[ i ].description != nullptr );
	}
#endif

	ss1_gameState.config        = com_get_config( SS1_CONFIG );
	ss1_gameState.isFirstLaunch = acm_get_bool( ss1_gameState.config, "isFirstLaunch", true );

	ss1_menu_initialize();

	ss1_gameState.camera = ape_create_camera( nullptr, nullptr, &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
	if ( ss1_gameState.camera == nullptr )
	{
		game_error_( "Failed to create player camera!\n" );
		return false;
	}

	return true;
}

static void serialize_config()
{
	acm_set_variable( ss1_gameState.config, "isFirstLaunch", ss1_gameState.isFirstLaunch ? "true" : "false", ACM_PROPERTY_TYPE_BOOL, true );

	com_write_config( ss1_gameState.config, SS1_CONFIG );
}

static bool ss1_shutdown()
{
	//TODO: need mechanism for removing components

	serialize_config();

	if ( ss1_gameState.camera != nullptr )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) ss1_gameState.camera );
		ss1_gameState.camera = nullptr;
	}

	if ( ss1_gameState.world != nullptr )
	{
		ape_world_node_destroy( ( ApeWorldNode * ) ss1_gameState.world );
		ss1_gameState.world = nullptr;
	}

	ss1_menu_shutdown();

	game_integrations_discord_shutdown_();

	return true;
}

static void handle_camera_input( double delta )
{
	if ( ss1_gameState.camera == nullptr )
	{
		return;
	}

	PLVector3 ang = ape_camera_get_angles( ss1_gameState.camera );
	PLVector3 pos = ape_camera_get_position( ss1_gameState.camera );

	switch ( ss1_gameState.cameraState )
	{
		default:
		case SS1_CAMERA_STATE_FREE:
		{
			PL_GET_CVAR( "input/mlook", mouseLook );
			if ( mouseLook != NULL && mouseLook->b_value )
			{
				int mx, my;
				ape_client_input_get_mouse_delta( &mx, &my );
				ang.y += ( float ) mx;
				ang.x += ( float ) my;
				ang.x = PlClamp( -90.0f, ang.x, 90.0f );
			}

			PLVector2 rightStick = ape_client_input_get_controller_axis_state( 0, 1 );
			ang.x -= ( rightStick.y * 100.0f ) * delta;
			ang.y -= ( rightStick.x * 150.0f ) * delta;

			PLVector3 forward, left;
			PlAnglesAxes( ang, &left, nullptr, &forward );

			PLVector2 leftStick = ape_client_input_get_controller_axis_state( 0, 0 );
			pos                 = PlAddVector3( pos, PlScaleVector3F( forward, ( leftStick.y * 100.0f ) * delta ) );
			pos                 = PlAddVector3( pos, PlScaleVector3F( left, ( leftStick.x * 100.0f ) * delta ) );

			ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( ss1_gameState.camera ) );
			if ( room != nullptr )
			{
				PLCollisionSphere sphere = {};
				sphere.origin            = pos;
				sphere.radius            = 4.0f;

				ApeCollisionCollider collider = {};
				collider.type                 = APE_COLLISION_TYPE_SPHERE;
				collider.sphere               = &sphere;

				unsigned int              numHits;
				ApeCollisionIntersection *hits;
				if ( ( hits = ape_room_intersect( room, &collider, &numHits ) ) != nullptr )
				{
					for ( unsigned int i = 0; i < numHits; ++i )
					{
						if ( hits[ i ].face != nullptr )
						{
							PLCollisionPlane plane = {};
							plane.origin           = hits[ i ].face->bounds.absOrigin;
							plane.normal           = hits[ i ].face->normal;
							ape_draw_debug_plane( &plane, PL_COLOUR_RED, 32.0f );

							float penetrationDepth = sphere.radius - hits[ i ].distance;
							if ( penetrationDepth > 0.0f )
							{
								PLVector3 collisionDirection = PlNormalizeVector3( PlSubtractVector3( sphere.origin, hits[ i ].intersection ) );
								pos                          = PlAddVector3( pos, PlScaleVector3F( collisionDirection, penetrationDepth ) );
							}
						}

						ape_draw_debug_axis( hits[ i ].intersection, pl_vecOrigin3, 2.0f );
					}

					PL_DELETE( hits );
				}
			}

			ape_camera_set_position( ss1_gameState.camera, &pos );
			ape_camera_set_angles( ss1_gameState.camera, &ang );
			break;
		}
		case SS1_CAMERA_STATE_FIRST_PERSON: break;
		case SS1_CAMERA_STATE_THIRD_PERSON:
		{
			ApeEntity *entity = game_server_get_host_entity_();
			if ( entity == nullptr )
			{
				return;
			}

			SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( entity );
			if ( playerEntity == nullptr )
			{
				game_warning_( "Player is possessing an entity that isn't a player!\n" );
				return;
			}

			PLVector2 rightStick = ape_client_input_get_controller_axis_state( 0, 1 );

			// update the pitch
			playerEntity->cameraAngles.x -= ( rightStick.y * 100.0f ) * delta;
			if ( playerEntity->cameraAngles.x > 90.0f )
			{
				playerEntity->cameraAngles.x = 90.0f;
			}
			else if ( playerEntity->cameraAngles.x < -90.0f )
			{
				playerEntity->cameraAngles.x = -90.0f;
			}

			// and the yaw
			playerEntity->cameraAngles.y -= ( rightStick.x * 150.0f ) * delta;

			com_math_normalize_angles( &playerEntity->cameraAngles, &playerEntity->cameraAngles );
			break;
		}
	}
}

static void handle_input( double delta )
{
	handle_camera_input( delta );

	ApeEntity *entity = game_server_get_host_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	GameMovementComponent *movementComponent = ape_entity_get_component( entity, "movement" );
	if ( movementComponent == nullptr )
	{
		game_warning_( "Player is possessing an entity without a movement component!\n" );
		return;
	}
}

static void camera_tick( double delta )
{
	ape_audio_clear_listener();

	ApeCamera *camera = ss1_gameState.camera;
	if ( camera == nullptr )
	{
		return;
	}

	if ( ss1_gameState.cameraState == ss1_gameState.oldCameraState )
	{
		// probably not transitioning between states...
		return;
	}

	PLVector3 cpos = ape_camera_get_position( camera );
	PLVector3 cang = ape_camera_get_angles( camera );

	ss1_gameState.oldCameraPosition = cpos;

	if ( ss1_gameState.cameraState == SS1_CAMERA_STATE_THIRD_PERSON )
	{
		ApeEntity *entity = game_server_get_host_entity_();
		if ( entity == nullptr )
		{
			return;
		}

		if ( strcmp( entity->classDefinition->name, "ss1_player" ) != 0 )
		{
			game_warning_( "Player is possessing an entity that isn't a player!\n" );
			return;
		}

		SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( entity );

		// entity camera position + view height
		PLVector3 epos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
		epos.y         = epos.y + playerEntity->cameraHeight;
		// entity camera angles
		PLVector3 eang = playerEntity->cameraAngles;

		PLVector3 forward, left;
		PlAnglesAxes( eang, &left, nullptr, &forward );

		// push entity position out and to either side
		PLVector3 npos = epos;
		npos           = PlAddVector3( npos, PlScaleVector3F( forward, playerEntity->cameraDistance ) );
		npos           = PlAddVector3( npos, PlScaleVector3F( left, playerEntity->cameraSide ) );

		// now interpolate the position and angles for the camera to the new position
		cpos = PlLinearInterpolateV3f( cpos, npos, 7.0f * delta );
		com_math_interpolate_angles( &cang, &eang, 7.0f * delta, &cang );

		// if the camera is hitting anything, move it
		ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( camera ) );
		if ( room != nullptr )
		{
			PLCollisionRay ray = {};
			ray.origin         = epos;
			ray.direction      = PlSubtractVector3( npos, epos );

			ApeCollisionIntersection result = {};
			if ( ape_room_ray_intersect( room, &ray, &result ) && result.distance <= playerEntity->cameraDistance )
			{
				cpos = result.intersection;
			}
		}

		ape_camera_set_position( camera, &cpos );
		ape_camera_set_angles( camera, &cang );
	}

	// this is utterly dumb, but we'll use this to determine a vague "velocity"
	PLVector3 cdiff = PlSubtractVector3( cpos, ss1_gameState.oldCameraPosition );

	ape_audio_update_listener( &cpos, &cang, &cdiff );
}

static void world_tick( double delta )
{
	ApeWorld *world = game_get_current_world();
	if ( world == nullptr )
	{
		return;
	}

	camera_tick( delta );

	GameWorldSimulation *simulation = &ss1_gameState.simulation;
	game_world_simulation_tick( simulation, delta );

	if ( ss1_gameState.sunLight != nullptr )
	{
		PLColourF32 sunColour  = SS1_DEFAULT_SUN_COLOUR;
		PLColourF32 moonColour = SS1_DEFAULT_MOON_COLOUR;

		ss1_gameState.sunAngles.x = game_world_simulation_get_seconds_in_day( simulation ) / ( game_world_simulation_get_seconds_to_day( simulation ) / 360.0f );
		ss1_gameState.sunAngles.y = sinf( PL_DEG2RAD( ss1_gameState.sunAngles.x + 90.0f ) ) * 2.0f;
		sunColour.a               = PlClamp( 0.0f, ( -ss1_gameState.sunAngles.y ) / 1.0f, 1.0f );

		PLVector3 sunPosition = com_math_pitch_yaw_to_position( ss1_gameState.sunAngles.y, ss1_gameState.sunAngles.x );
		ape_light_set_position( ss1_gameState.sunLight, &sunPosition );
		ape_light_set_colour( ss1_gameState.sunLight, &sunColour );

		PLVector3 moonPosition = com_math_pitch_yaw_to_position( -ss1_gameState.sunAngles.y, -ss1_gameState.sunAngles.x );
		ape_light_set_position( ss1_gameState.moonLight, &moonPosition );
		moonColour.a = PlClamp( 0.0f, ( ss1_gameState.sunAngles.y ) / 1.0f, 0.25f );
		ape_light_set_colour( ss1_gameState.moonLight, &moonColour );

		ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( ss1_gameState.sunLight ) );
		if ( room != nullptr )
		{
			PLColourF32 ambience;
			ambience.r = PlClamp( 0.05f, sunColour.r * ( sunColour.a / 0.5f ), 0.45f );
			ambience.g = PlClamp( 0.05f, sunColour.r * ( sunColour.a / 0.5f ), 0.45f );
			ambience.b = PlClamp( 0.05f, sunColour.r * ( sunColour.a / 0.5f ), 0.45f );
			ambience.a = 1.0f;

			ape_room_set_ambience( room, ambience );

			ApeViewport *viewport = ape_viewport_get_active();
			if ( viewport != nullptr )
			{
				// BLEH
				PLColour bamb = PlColourF32ToU8( &ambience );
				ape_viewport_set_clear_colour( viewport, &bamb );
			}
		}
	}
}

static void test_cylinder_point_collision()
{
	if ( ss1_gameState.camera == nullptr )
	{
		return;
	}

	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = PL_VECTOR3( 16.0f, 16.0f, 32.0f );
	cylinder.radius               = 16.0f;

	PLVector3 point = ape_camera_get_position( ss1_gameState.camera );

	PLColour colour;
	if ( com_collision_cylinder_intersect_point( &cylinder, &point ) )
	{
		colour = PL_COLOURU8( 0, 255, 0, 255 );
	}
	else
	{
		colour = PL_COLOURU8( 255, 0, 0, 255 );
	}

	ape_draw_debug_cylinder( &cylinder, &colour, 16 );
}

static void test_cylinder_polygon_collision()
{
	PLVector3 point = ape_camera_get_position( ss1_gameState.camera );
	point.y -= 32.0f;

	ComCollisionCylinder cylinder = {};
	cylinder.height               = 64.0f;
	cylinder.origin               = point;
	cylinder.radius               = 16.0f;

	static constexpr PLVector3 vertices[] = {
	        PL_VECTOR3( 0.0f, 16.0f, 0.0f ),
	        PL_VECTOR3( 32.0f, 16.0f, 0.0f ),
	        PL_VECTOR3( 32.0f, 16.0f, 32.0f ),
	        PL_VECTOR3( 0.0f, 16.0f, 32.0f ),
	};
	static constexpr unsigned int numVertices = PL_ARRAY_ELEMENTS( vertices );

	PLColour colour;
	if ( com_collision_cylinder_intersect_polygon( &cylinder, vertices, numVertices, &PL_VECTOR3( 0.0f, 1.0f, 0.0f ) ) )
	{
		colour = PL_COLOURU8( 0, 255, 0, 255 );
	}
	else
	{
		colour = PL_COLOURU8( 255, 0, 0, 255 );
	}

	ape_draw_debug_polygon( vertices, numVertices, colour );
	ape_draw_debug_cylinder( &cylinder, &colour, 16 );
}

static void ss1_tick( double delta )
{
	delta = game_get_time_delta_( delta );

	game_server_tick_( delta );

	world_tick( delta );

	//test_cylinder_point_collision();
	test_cylinder_polygon_collision();
}

static bool ss1_draw( ApeViewport *viewport )
{
	ape_camera_make_active( ss1_gameState.camera );
	ape_camera_draw_perspective( ss1_gameState.camera, viewport );
	return true;
}

static bool ss1_draw_menu( const ApeViewport *viewport )
{
	ss1_menu_draw( viewport );
	return true;
}

static void ss1_spawn_world( ApeRoom *room )
{
	game_world_simulation_initialize( &ss1_gameState.simulation );

	game_team_init( SS1_MAX_TEAMS );

	game_menu_set_active( nullptr );

	// setup the team resource pools
	for ( unsigned int i = 0; i < SS1_MAX_TEAMS; ++i )
	{
		PL_ZERO_( teamResourcePools[ i ] );
		game_team_set_resource_pools( i, teamResourcePools[ i ], SS1_MAX_RESOURCE_TYPES );
	}

	ApeWorldNode *roomNode = APE_WORLD_NODE( room );
	ape_world_node_attach( APE_WORLD_NODE( ss1_gameState.camera ), roomNode );
	ape_world_node_set_position( APE_WORLD_NODE( ss1_gameState.camera ), &PL_VECTOR3( 0.0f, 128.0f, 0.0f ) );

	//TODO: make this configurable via editor?
	ss1_gameState.simulation.seconds = 40000;

#if 0
	//TODO: these shouldn't be hard-coded this way as we might not want our level to have sun/moon lights
	ss1_gameState.sunLight  = ape_create_light( roomNode, &SS1_DEFAULT_SUN_POSITION, &SS1_DEFAULT_SUN_COLOUR, 0.0f,
	                                            APE_LIGHT_TYPE_SUN,
	                                            APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC | APE_LIGHT_FLAG_RUNTIME_SHADOWS );
	ss1_gameState.moonLight = ape_create_light( roomNode, &SS1_DEFAULT_SUN_POSITION, &SS1_DEFAULT_MOON_COLOUR, 0.0f,
	                                            APE_LIGHT_TYPE_SUN,
	                                            APE_LIGHT_FLAG_ENABLED | APE_LIGHT_FLAG_DYNAMIC | APE_LIGHT_FLAG_RUNTIME_SHADOWS );
#endif

	//ape_entity_create( roomNode, "ss1_airship", "airship_0", nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );

	const char *path = ape_world_node_get_path( APE_WORLD_NODE( room ) );
	if ( *path == '\0' )
	{
		path = "unknown";
	}
	const char *c = G_STR_( "Invading %s" );
	char        buf[ 128 ];
	snprintf( buf, sizeof( buf ), c, path );
	game_integrations_discord_update_activity_( buf, nullptr, "qm1-logo", SS1_GAME_TITLE );
}

static void on_destroy_room( ApeRoom *room )
{
	ApeWorldNode *worldNode = ape_world_node_get_parent( APE_WORLD_NODE( ss1_gameState.camera ) );
	if ( worldNode == APE_WORLD_NODE( room ) )
	{
		// save the camera from being destroyed!
		ape_world_node_dettach( APE_WORLD_NODE( ss1_gameState.camera ) );
	}
}

static bool request_handler( ApeGameInterfaceRequest gameModeRequest, void *user )
{
	switch ( gameModeRequest )
	{
		case APE_GAME_INTERFACE_REQUEST_INITIALIZE:
			return ss1_initialize();
		case APE_GAME_INTERFACE_REQUEST_SHUTDOWN:
			return ss1_shutdown();
		case APE_GAME_INTERFACE_REQUEST_DRAW:
			return ss1_draw( user );
		case APE_GAME_INTERFACE_REQUEST_DRAW_UI:
			return ss1_draw_menu( user );
		default:
			break;
	}

	return false;
}

static bool server_client_validate( ApeServerClient *clientHandle )
{
	return game_server_client_validate_( clientHandle );
}

static void server_client_connected( ApeServerClient *clientHandle )
{
	game_server_client_connected_( clientHandle );
}

static void server_client_disconnected( ApeServerClient *clientHandle )
{
	game_server_client_disconnected_( clientHandle );
}

static void server_process_message( ApeServerClient *clientHandle, const void *buf, size_t bufSize )
{
	game_server_process_message_( clientHandle, buf, bufSize );
}

static void client_process_message( const void *buf, size_t bufSize )
{
	game_client_process_message_( buf, bufSize );
}

static void client_tick( double delta )
{
	ss1_menu_tick( delta );

	handle_input( delta );

	game_integrations_discord_tick_();
}

const ApeGameInterfaceImport *ape_game_get_interface( void )
{
	static ApeGameInterfaceImport gameMode = {
	        .version               = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion       = SS1_GAME_PROTOCOL_VERSION + GAME_NET_PROTOCOL_VERSION,
	        .identifier            = "ss1",
	        .requestCallbackMethod = request_handler,
	        .spawnWorld            = ss1_spawn_world,

	        .onDestroyRoom = on_destroy_room,

	        .serverClientValidate     = server_client_validate,
	        .serverClientConnected    = server_client_connected,
	        .serverClientDisconnected = server_client_disconnected,
	        .serverProcessMessage     = server_process_message,
	        .serverTick               = ss1_tick,

	        .clientProcessMessage = client_process_message,
	        .clientTick           = client_tick,
	};
	return &gameMode;
}
