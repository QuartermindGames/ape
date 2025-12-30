// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_string.h"

#include "ss1_game.h"
#include "qm1_menu.h"

#include "integrations/integrations.h"
#include "physics/physics.h"
#include "game_team.h"
#include "components/component_movement.h"

#include "entities/entity_player_spawn.h"
#include "entities/qm1/qm1_entity_player.h"

SS1GameState ss1_gameState;

#define SS1_CONFIG "game_ss1"

static unsigned int teamResourcePools[ QM1_GAME_MAX_TEAMS ][ SS1_MAX_RESOURCE_TYPES ];

void ss1_actions_register_();

static void damage_player_command( [[maybe_unused]] const unsigned int argc, char **argv )
{
	int16_t value;
	if ( argc > 1 )
	{
		value = ( int16_t ) atoi( argv[ 1 ] );
	}
	else
	{
		value = 10;
	}

	//TODO: pump the event...
}

static void print_camera_pos_command( unsigned int argc, char **argv )
{
	if ( ss1_gameState.camera == nullptr )
	{
		game_print_( "No valid camera.\n" );
		return;
	}

	char tmp[ 64 ];

	QmMathVector3f cameraPos = ape_camera_get_position( ss1_gameState.camera );
	game_print_( "Camera Pos: %s\n", qm_math_vector3f_print( cameraPos, tmp, sizeof( tmp ) ) );
	QmMathVector3f cameraAngles = ape_camera_get_angles( ss1_gameState.camera );
	game_print_( "Camera Ang: %s\n", qm_math_vector3f_print( cameraAngles, tmp, sizeof( tmp ) ) );
}

static void camera_save_pos_command( [[maybe_unused]] unsigned int argc, [[maybe_unused]] char **argv )
{
	if ( ss1_gameState.camera == nullptr )
	{
		return;
	}

	QmMathVector3f pos = ape_camera_get_position( ss1_gameState.camera );
	QmMathVector3f ang = ape_camera_get_angles( ss1_gameState.camera );

	char *path = qm_os_string_alloc( nullptr, "%s/camera.dat", com_get_app_data_directory() );
	FILE *file = fopen( path, "w" );
	if ( file != nullptr )
	{
		fprintf( file, "%f %f %f %f %f %f",
		         pos.x, pos.y, pos.z,
		         ang.x, ang.y, ang.z );
		fclose( file );
	}
	else
	{
		game_warning_( "Failed to create camera file (%s)!\n", path );
	}

	qm_os_memory_free( path );
}

static void camera_restore_pos_command( [[maybe_unused]] unsigned int argc, [[maybe_unused]] char **argv )
{
	if ( ss1_gameState.camera == nullptr )
	{
		return;
	}

	char *path = qm_os_string_alloc( nullptr, "%s/camera.dat", com_get_app_data_directory() );
	FILE *file = fopen( path, "r" );
	if ( file != nullptr )
	{
		QmMathVector3f pos = {};
		QmMathVector3f ang = {};

		fscanf( file, "%f %f %f %f %f %f",
		        &pos.x, &pos.y, &pos.z,
		        &ang.x, &ang.y, &ang.z );

		ape_camera_set_position( ss1_gameState.camera, &pos );
		ape_camera_set_angles( ss1_gameState.camera, &ang );
	}
	else
	{
		game_warning_( "Failed to open camera file (%s)!\n", path );
	}

	qm_os_memory_free( path );
}

extern ApeEntityClassDefinition ss1_airshipEntityClass;
extern ApeEntityClassDefinition ss1_pawnEntityClass;
extern ApeEntityClassDefinition ss1_playerEntityClass;

static bool ss1_initialize()
{
	if ( !game_initialize_() )
	{
		return false;
	}

	ss1_actions_register_();

	ape_register_entity_class( &ss1_airshipEntityClass );
	ape_register_entity_class( &ss1_pawnEntityClass );
	ape_register_entity_class( &ss1_playerEntityClass );

	PL_ZERO_( ss1_gameState );

	PlRegisterConsoleCommand( "qm1_damage_player", "Damage the player by a specific amount.", -1, damage_player_command );
	PlRegisterConsoleCommand( "qm1_print_camera_pos", "Print the camera position and angles.", 0, print_camera_pos_command );
	PlRegisterConsoleCommand( "qm1_camera_save_pos", "Save the current camera position.", 0, camera_save_pos_command );
	PlRegisterConsoleCommand( "qm1_camera_restore_pos", "Restore the camera position.", 0, camera_restore_pos_command );

#if !defined( NDEBUG )
	// validate all the professions are setup correctly
	for ( unsigned int i = 0; i < QM1_CHARACTER_MAX_PROFESSIONS; ++i )
	{
		assert( qm1_professions_[ i ].name != nullptr && qm1_professions_[ i ].description != nullptr );
	}
#endif

	ss1_gameState.config        = com_get_config( SS1_CONFIG );
	ss1_gameState.isFirstLaunch = acm_get_bool( ss1_gameState.config, "isFirstLaunch", true );

	ss1_menu_initialize_();

	ss1_gameState.camera = ape_create_camera( nullptr, nullptr, &pl_vecOrigin3, &pl_vecOrigin3, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
	if ( ss1_gameState.camera == nullptr )
	{
		game_error_( "Failed to create player camera!\n" );
		return false;
	}

	ss1_gameState.cameraState = GAME_CAMERA_STATE_FIXED;

	return true;
}

static void serialize_config()
{
	acm_set_variable( ss1_gameState.config, "isFirstLaunch", ss1_gameState.isFirstLaunch ? "true" : "false", ACM_PROPERTY_TYPE_BOOL, true );

	com_write_config( ss1_gameState.config, SS1_CONFIG );
}

static void ss1_shutdown()
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

	ss1_menu_shutdown_();

	game_shutdown_();
}

static void handle_camera_input( double delta )
{
	if ( ss1_gameState.camera == nullptr )
	{
		return;
	}

	QmMathVector3f ang = ape_camera_get_angles( ss1_gameState.camera );
	QmMathVector3f pos = ape_camera_get_position( ss1_gameState.camera );

	switch ( ss1_gameState.cameraState )
	{
		default:
		case GAME_CAMERA_STATE_FREE:
		{
			PL_GET_CVAR( "input/mlook", mouseLook );
			if ( mouseLook != NULL && mouseLook->b_value )
			{
				int mx, my;
				ape_client_input_get_mouse_delta( &mx, &my );
				ang.y += ( float ) mx;
				ang.x += ( float ) my;
				ang.x = QM_MATH_CLAMP( -90.0f, ang.x, 90.0f );
			}

			QmMathVector2f rightStick = ape_client_input_get_controller_axis_state( 0, 1 );
			ang.x -= ( rightStick.y * 100.0f ) * delta;
			ang.y -= ( rightStick.x * 150.0f ) * delta;

			QmMathVector3f forward, left;
			PlAnglesAxes( ang, &left, nullptr, &forward );

			QmMathVector2f leftStick = ape_client_input_get_controller_axis_state( 0, 0 );
			pos                      = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( forward, ( leftStick.y * 100.0f ) * delta ) );
			pos                      = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( left, ( leftStick.x * 100.0f ) * delta ) );

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
								QmMathVector3f collisionDirection = qm_math_vector3f_normalize( qm_math_vector3f_sub( sphere.origin, hits[ i ].intersection ) );
								pos                               = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( collisionDirection, penetrationDepth ) );
							}
						}

						ape_draw_debug_axis( hits[ i ].intersection, pl_vecOrigin3, 2.0f );
					}

					qm_os_memory_free( hits );
				}
			}

			ape_camera_set_position( ss1_gameState.camera, &pos );
			ape_camera_set_angles( ss1_gameState.camera, &ang );
			break;
		}

		case GAME_CAMERA_STATE_FIRST_PERSON:
		case GAME_CAMERA_STATE_THIRD_PERSON:
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

			QmMathVector2f rightStick = ape_client_input_get_controller_axis_state( 0, 1 );

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

static void camera_third_person_tick( ApeCamera *camera, const double delta )
{
	ApeEntity *entity = game_server_get_host_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( entity );
	if ( playerEntity == nullptr )
	{
		return;
	}

	QmMathVector3f cpos = ape_camera_get_position( camera );
	QmMathVector3f cang = ape_camera_get_angles( camera );

	// entity camera position + view height
	QmMathVector3f epos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
	epos.y              = epos.y + playerEntity->cameraHeight;
	// entity camera angles
	QmMathVector3f eang = playerEntity->cameraAngles;

	QmMathVector3f forward, left;
	PlAnglesAxes( eang, &left, nullptr, &forward );

	// push entity position out and to either side
	QmMathVector3f npos = epos;
	npos                = qm_math_vector3f_add( npos, qm_math_vector3f_scale_float( forward, playerEntity->cameraDistance ) );
	npos                = qm_math_vector3f_add( npos, qm_math_vector3f_scale_float( left, playerEntity->cameraSide ) );

	// now interpolate the position and angles for the camera to the new position
	cpos = PlLinearInterpolateV3f( cpos, npos, 7.0f * delta );
	com_math_interpolate_angles( &cang, &eang, 7.0f * delta, &cang );

	// if the camera is hitting anything, move it
	ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( camera ) );
	if ( room != nullptr )
	{
		PLCollisionRay ray = {};
		ray.origin         = epos;
		ray.direction      = qm_math_vector3f_sub( npos, epos );

		ApeCollisionIntersection result = {};
		if ( ape_room_ray_intersect( room, &ray, &result ) && result.distance <= playerEntity->cameraDistance )
		{
			cpos = result.intersection;
		}
	}

	ape_camera_set_position( camera, &cpos );
	ape_camera_set_angles( camera, &cang );
}

static void camera_first_person_tick( ApeCamera *camera, const double delta )
{
	ApeEntity *entity = game_server_get_host_entity_();
	if ( entity == nullptr )
	{
		return;
	}

	SS1PlayerEntity *playerEntity = SS1_PLAYER_ENTITY( entity );
	if ( playerEntity == nullptr )
	{
		return;
	}

	QmMathVector3f cpos = ape_camera_get_position( camera );
	QmMathVector3f cang = ape_camera_get_angles( camera );

	// entity camera position + view height
	QmMathVector3f epos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
	epos.y              = epos.y + playerEntity->cameraHeight;

	// entity camera angles
	QmMathVector3f eang = playerEntity->cameraAngles;

	QmMathVector3f forward, left;
	PlAnglesAxes( eang, &left, nullptr, &forward );

	// now interpolate the position and angles for the camera to the new position
	cpos = PlLinearInterpolateV3f( cpos, epos, 7.0f * delta );
	com_math_interpolate_angles( &cang, &eang, 16.0f * delta, &cang );

	ape_camera_set_position( camera, &cpos );
	ape_camera_set_angles( camera, &cang );
}

static void camera_tick( const double delta )
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

	QmMathVector3f cpos = ape_camera_get_position( camera );
	QmMathVector3f cang = ape_camera_get_angles( camera );

	ss1_gameState.oldCameraPosition = cpos;

	switch ( ss1_gameState.cameraState )
	{
		default:
			break;
		case GAME_CAMERA_STATE_FIRST_PERSON:
			camera_first_person_tick( camera, delta );
			break;
		case GAME_CAMERA_STATE_THIRD_PERSON:
			camera_third_person_tick( camera, delta );
			break;
	}

	// this is utterly dumb, but we'll use this to determine a vague "velocity"
	QmMathVector3f cdiff = qm_math_vector3f_sub( cpos, ss1_gameState.oldCameraPosition );

	ape_audio_update_listener( &cpos, &cang, &cdiff );
}

static void world_tick( const double delta )
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
		QmMathColour4f sunColour  = SS1_DEFAULT_SUN_COLOUR;
		QmMathColour4f moonColour = SS1_DEFAULT_MOON_COLOUR;

		ss1_gameState.sunAngles.x = game_world_simulation_get_seconds_in_day( simulation ) / ( game_world_simulation_get_seconds_to_day( simulation ) / 360.0f );
		ss1_gameState.sunAngles.y = sinf( PL_DEG2RAD( ss1_gameState.sunAngles.x + 90.0f ) ) * 2.0f;
		sunColour.a               = QM_MATH_CLAMP( 0.0f, ( -ss1_gameState.sunAngles.y ) / 1.0f, 1.0f );

		QmMathVector3f sunPosition = com_math_pitch_yaw_to_position( ss1_gameState.sunAngles.y, ss1_gameState.sunAngles.x );
		ape_light_set_position( ss1_gameState.sunLight, &sunPosition );
		ape_light_set_colour( ss1_gameState.sunLight, &sunColour );

		QmMathVector3f moonPosition = com_math_pitch_yaw_to_position( -ss1_gameState.sunAngles.y, -ss1_gameState.sunAngles.x );
		ape_light_set_position( ss1_gameState.moonLight, &moonPosition );
		moonColour.a = QM_MATH_CLAMP( 0.0f, ( ss1_gameState.sunAngles.y ) / 1.0f, 0.25f );
		ape_light_set_colour( ss1_gameState.moonLight, &moonColour );

		ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( ss1_gameState.sunLight ) );
		if ( room != nullptr )
		{
			QmMathColour4f ambience;
			ambience.r = QM_MATH_CLAMP( 0.05f, sunColour.r * ( sunColour.a / 0.5f ), 0.45f );
			ambience.g = QM_MATH_CLAMP( 0.05f, sunColour.r * ( sunColour.a / 0.5f ), 0.45f );
			ambience.b = QM_MATH_CLAMP( 0.05f, sunColour.r * ( sunColour.a / 0.5f ), 0.45f );
			ambience.a = 1.0f;

			ape_room_set_ambience( room, ambience );

			ApeViewport *viewport = ape_viewport_get_active();
			if ( viewport != nullptr )
			{
				// BLEH
				QmMathColour4ub bamb = PlColourF32ToU8( &ambience );
				ape_viewport_set_clear_colour( viewport, &bamb );
			}
		}
	}
}

static void ss1_tick( double delta )
{
	delta = game_get_delta_mod_( delta );

	game_server_tick_( delta );

	world_tick( delta );

#if 0
	if ( ss1_gameState.camera != nullptr )
	{
		QmMathVector3f cameraPos = ape_camera_get_position( ss1_gameState.camera );
		game_test_cylinder_point_collision_( &cameraPos );
		game_test_cylinder_polygon_collision_( &cameraPos );
	}
#endif
}

static void ss1_draw( const ApeViewport *viewport )
{
	ape_camera_make_active( ss1_gameState.camera );
	ape_camera_draw_perspective( ss1_gameState.camera, viewport );
}

static void ss1_draw_menu( const ApeViewport *viewport )
{
	ss1_menu_draw( viewport );
}

static void spawn_characters()
{
	// create all the characters for each spawn point
	PLLinkedList *playerSpawns = game_player_spawn_get_spawn_points();
	if ( playerSpawns == nullptr )
	{
		game_warning_( "Unable to spawn player entities, no spawn points!\n" );
		return;
	}

	ApeEntity *spawnEntity;
	COM_ITERATE_LINKED_LIST( spawnEntity, playerSpawns, i )
	{
		ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( spawnEntity ) );
		if ( room == nullptr )
		{
			game_warning_( "Encountered a player spawn outside a room!\n" );
			continue;
		}

		QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( spawnEntity ) );
		QmMathVector3f ang = ape_world_node_get_angles( APE_WORLD_NODE( spawnEntity ) );

		ape_entity_create( APE_WORLD_NODE( room ), "ss1_player", "player", nullptr, &pos, &ang );
	}
}

static void ss1_spawn_world( ApeRoom *room )
{
	game_world_simulation_initialize( &ss1_gameState.simulation );

	game_team_init( QM1_GAME_MAX_TEAMS );

	game_menu_set_active( nullptr );

	// setup the team resource pools
	for ( unsigned int i = 0; i < QM1_GAME_MAX_TEAMS; ++i )
	{
		PL_ZERO_( teamResourcePools[ i ] );
		game_team_set_resource_pools( i, teamResourcePools[ i ], SS1_MAX_RESOURCE_TYPES );
	}

	ApeWorldNode *roomNode = APE_WORLD_NODE( room );
	ape_world_node_attach( APE_WORLD_NODE( ss1_gameState.camera ), roomNode );
	ape_world_node_set_position( APE_WORLD_NODE( ss1_gameState.camera ), &QM_MATH_VECTOR3F( 0.0f, 128.0f, 0.0f ) );

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

	ape_entity_create( roomNode, "ss1_airship", "airship_0", nullptr, &pl_vecOrigin3, &pl_vecOrigin3 );

	const char *path = ape_world_node_get_path( APE_WORLD_NODE( room ) );
	if ( *path == '\0' )
	{
		path = "unknown";
	}
	const char *c = G_STR_( "Invading %s" );
	char        buf[ 128 ];
	snprintf( buf, sizeof( buf ), c, path );
	game_integrations_discord_update_activity_( buf, nullptr, "qm1-logo", QM1_GAME_TITLE );

	spawn_characters();

	ss1_gameState.roundStatus = QM1_ROUND_STATUS_INTRO;
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
	        .version         = APE_GAME_INTERFACE_VERSION,
	        .protocolVersion = SS1_GAME_PROTOCOL_VERSION,
	        .identifier      = "ss1",

	        .initialize = ss1_initialize,
	        .shutdown   = ss1_shutdown,
	        .draw       = ss1_draw,
	        .drawUI     = ss1_draw_menu,

	        .spawnWorld = ss1_spawn_world,

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
