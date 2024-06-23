// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl_console.h>
#include <plcore/pl_physics.h>
#include <plcore/pl_linkedlist.h>
#include <plcore/pl_hashtable.h>

#include "yin/core.h"
#include "yin/core_entity.h"
#include "yin/core_input.h"
#include "yin/core_game.h"
#include "yin/gui_public.h"
#include "yin/node.h"

#include "ape/ape_public_client.h"

#include "game/game_public.h"

PL_EXTERN_C

void ss_game_register_standard_entity_components_( void );

void game_interface_import_setup_( ApeGameInterfaceImport *import, unsigned int version, unsigned int protocolVersion, const char *id );

void game_print_( const char *message, ... );
void game_warning_( const char *message, ... );
void game_error_( const char *message, ... );

#if !defined( NDEBUG )
void game_debug_( const char *message, ... );
#else
#	define game_debug_( ... )
#endif

/////////////////////////////////////////////////////////////////

#define GAME_NET_PROTOCOL_VERSION 1

typedef enum GameNetMessageType : uint16_t
{
	GAME_NET_MESSAGE_SAY,
} GameNetMessageType;

#define GAME_NET_MAX_SAY_MESSAGE UINT8_MAX

typedef struct __attribute( ( packed ) ) GameNetMessageHeader
{
	GameNetMessageType type;
} GameNetMessageHeader;

/////////////////////////////////////////////////////////////////

PL_EXTERN_C_END
