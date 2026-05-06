// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "game_private.h"
#include "game_server.h"
#include "game_client.h"
#include "game_world_simulation.h"

static constexpr char NIH_GAME_TITLE[] = "Nihlexa";

static constexpr unsigned int NIH_GAME_PROTOCOL_VERSION = GAME_NET_PROTOCOL_VERSION + 1;

static constexpr unsigned int NIH_GAME_MAX_TEAMS      = 4;
static constexpr unsigned int NIH_GAME_MAX_PLAYERS    = 4;
static constexpr unsigned int NIH_GAME_MAX_CHARACTERS = 8;

typedef enum Qm1RoundStatus
{
	QM1_ROUND_STATUS_INTRO,
	QM1_ROUND_STATUS_SELECT,
	QM1_ROUND_STATUS_SELECTED,
	QM1_ROUND_STATUS_PLAYING,
	QM1_ROUND_STATUS_END,
} Qm1RoundStatus;

typedef struct NihGameState
{
	GamePlayer players[ NIH_GAME_MAX_PLAYERS ];

	ApeCamera     *camera;// our eyes
	QmMathVector3f oldCameraPosition;

	Qm1RoundStatus roundStatus;

	ApeRoom  *spawnRoom;// cockpit
	ApeRoom  *playRoom; // world we're actually playing in
	ApeWorld *world;    // world container

	bool       isFirstLaunch;
	AcmBranch *config;
} NihGameState;
extern NihGameState qm1_state_;

bool qm1_world_setup_();
void qm1_world_spawn_( ApeRoom *room );
