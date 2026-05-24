// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Nihlexa client-side logic.
// Author:  Mark E. Sowden

#include "nihlexa.h"
#include "nihlexa_menu.h"
#include "components/component_camera.h"

#include "integrations/integrations.h"

NihClientState nih_clientState_;

void nih_client_connected_()
{
	game_integrations_discord_update_activity_( "Preparing", nullptr, "qm1-logo", NIH_GAME_TITLE );

	GamePlayer *player = game_server_get_local_player_();
	if ( player != nullptr && player->camera == nullptr )
	{
		player->camera = ape_create_camera( nullptr, "player_camera", &QM_MATH_VECTOR3F_ZERO, &QM_MATH_VECTOR3F_ZERO, APE_CAMERA_MODE_PERSPECTIVE, APE_CAMERA_DRAW_MODE_SHADED );
		if ( player->camera == nullptr )
		{
			game_warning_( "Failed to create player camera on connect!\n" );
			ape_client_disconnect();
			return;
		}

		if ( player->entity != nullptr )
		{
			ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( player->entity ) );
			ape_world_node_attach( APE_WORLD_NODE( player->camera ), APE_WORLD_NODE( room ) );

			GameCameraComponent *cameraComponent = ape_entity_get_component( player->entity, GAME_CAMERA_COMPONENT_NAME );
			if ( cameraComponent != nullptr )
			{
				game_component_camera_set_state_( cameraComponent, GAME_CAMERA_STATE_FIRST_PERSON );
			}
		}
	}
}

void nih_client_tick_( const double delta )
{
	nih_menu_tick( delta );

	// try to tick the local camera
	//TODO: this might need moving server-side anyway, but for now let's just get this crap working
	GamePlayer *player = game_server_get_local_player_();
	if ( player != nullptr && player->camera != nullptr )
	{
		ApeEntity *entity = game_server_get_local_entity_();
		if ( entity != nullptr )
		{
			GameCameraComponent *cameraComponent = ape_entity_get_component( entity, GAME_CAMERA_COMPONENT_NAME );
			if ( cameraComponent != nullptr )
			{
				QmMathVector3f entityPos = ape_world_node_get_position( APE_WORLD_NODE( entity ) );
				game_component_camera_tick_( cameraComponent, player->camera, entityPos, delta );
			}
		}

#if 0
		ApeRoom *room = ape_world_node_get_room( APE_WORLD_NODE( player->camera ) );

		ComCollisionCylinder cylinder = {};
		cylinder.height               = 32.0f;
		cylinder.origin               = ape_camera_get_position( player->camera );
		cylinder.radius               = 16.0f;

		cylinder.origin.y -= 64.0f;

		ApeCollisionCollider collider = {};
		collider.type                 = APE_COLLISION_TYPE_CYLINDER;
		collider.cylinder             = &cylinder;

		ApeWorldNode *ignores[] = {
		        APE_WORLD_NODE( player->camera ),
		        APE_WORLD_NODE( entity ),
		};

		collider.ignores    = ignores;
		collider.numIgnores = QM_OS_ARRAY_ELEMENTS( ignores );

		unsigned int              numHits;
		ApeCollisionIntersection *hits;
		if ( ( hits = ape_room_intersect( room, &collider, &numHits ) ) != nullptr )
		{
			for ( unsigned int i = 0; i < numHits; ++i )
			{
				ape_draw_debug_arrow( cylinder.origin, hits[ i ].origin, PL_COLOUR_GREEN, 2.0f );
			}

			ape_draw_debug_cylinder( &cylinder, &PL_COLOUR_GREEN, 16 );
		}
		else
		{
			ape_draw_debug_cylinder( &cylinder, &PL_COLOUR_RED, 16 );
		}
#else

		QmMathVector3f cameraPos = ape_camera_get_position( player->camera );

		//game_test_cylinder_aabb_collision_( &cameraPos );
		//game_test_cylinder_point_collision_( &cameraPos );
		//game_test_cylinder_cylinder_collision_( &cameraPos );

#endif
	}

	game_integrations_discord_tick_();
}

void nih_client_draw_( const ApeViewport *viewport )
{
	GamePlayer *player = game_server_get_local_player_();
	if ( player == nullptr || player->camera == nullptr )
	{
		return;
	}

	ape_camera_make_active( player->camera );
	ape_camera_draw_perspective( player->camera, viewport );
}

void nih_client_draw_ui_( const ApeViewport *viewport )
{
	nih_menu_draw( viewport );
}
