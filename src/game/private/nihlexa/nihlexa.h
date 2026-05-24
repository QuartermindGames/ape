// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "game_private.h"
#include "game_server.h"
#include "game_client.h"
#include "game_world_simulation.h"

static constexpr char NIH_GAME_TITLE[]  = "Nihlexa";
static constexpr char NIH_GAME_CONFIG[] = "nihlexa";

static constexpr unsigned int NIH_GAME_PROTOCOL_VERSION = GAME_NET_PROTOCOL_VERSION + 1;

typedef enum NihGameMode : uint8_t
{
	NIH_GAME_MODE_SP,
	NIH_GAME_MODE_DM,
	NIH_GAME_MODE_TDM,
	NIH_GAME_MODE_CTF,
	NIH_GAME_MODE_SURVIVOR,// "mystical" mode

	NIH_GAME_MODE_MAX
} NihGameMode;

NihGameMode nih_get_game_mode();

typedef struct NihClientState
{
	ApeCamera *camera;
} NihClientState;
extern NihClientState nih_clientState_;

typedef struct NihServerState
{
	NihGameMode mode;

	ApeWorld *world;// world container

	bool       isFirstLaunch;
	AcmBranch *config;
} NihServerState;
extern NihServerState nih_serverState_;

void nih_world_spawn_( ApeRoom *room );
