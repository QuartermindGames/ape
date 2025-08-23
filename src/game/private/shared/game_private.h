// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "qmos/public/qm_os.h"

#include <plcore/pl_console.h>
#include <plcore/pl_physics.h>
#include <plcore/pl_linkedlist.h>
#include <plcore/pl_hashtable.h>

#include <acm/acm.h>

#include "common/common_math.h"

#include "yin/core.h"
#include "yin/core_entity.h"
#include "yin/core_input.h"
#include "yin/core_game.h"

#include "ape/ape_public_gui.h"
#include "ape/ape_public_client.h"

#include "game/game_public.h"

#include "game_language.h"
#include "game_team.h"

PL_EXTERN_C

typedef struct GameServerClient GameServerClient;

#define GAME_MAX_CLIENTS 128
#define GAME_MAX_PLAYERS 64

/**
 * Try to avoid using deltatime on its own, and
 * instead pass it through this function!
 *
 * Why is this not applied engine-side or more globally?
 * Because you likely don't want absolutely everything
 * to be influenced by the time modifier.
 *
 * @param delta Current deltatime value.
 * @return		Delta multiplied by the time modifier.
 */
double game_get_delta_mod_( double delta );

void game_print_( const char *message, ... );
void game_warning_( const char *message, ... );
void game_error_( const char *message, ... );

#if !defined( NDEBUG )
void game_debug_( const char *message, ... );
#else
#	define game_debug_( ... )
#endif

/**
 * Get the base game config.
 *
 * @return	Returns a pointer to the base game config.
 */
AcmBranch *game_get_config();

/////////////////////////////////////////////////////////////////

#define GAME_NET_PROTOCOL_VERSION 1

typedef enum GameNetMessageType : uint16_t
{
	GAME_NET_MESSAGE_SAY,     // message from a client
	GAME_NET_MESSAGE_ANNOUNCE,// an announcement broadcasted from the server
} GameNetMessageType;

#define GAME_NET_MAX_SAY_MESSAGE UINT8_MAX

typedef struct __attribute( ( packed ) ) GameNetMessageHeader
{
	GameNetMessageType type;
} GameNetMessageHeader;

typedef char GamePlayerName[ 64 ];
typedef struct GamePlayer
{
	GameServerClient *serverClient;// internal server-side client reference
	unsigned int      team;        // the team the player is associated with

	ApeEntity *entity;// target entity the player is controlling
} GamePlayer;

const char *game_player_get_name_( const GamePlayer *self );

/////////////////////////////////////////////////////////////////
// Test Methods

void game_test_cylinder_point_collision_( const PLVector3 *pos );
void game_test_cylinder_polygon_collision_( const PLVector3 *pos );

bool game_test_fire_decal_( ApeRoom *room, const PLVector3 *pos, const PLVector3 *dir );

PL_EXTERN_C_END
