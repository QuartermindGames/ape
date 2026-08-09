// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "components/component_camera.h"

typedef struct ApeModelNode ApeModelNode;

typedef struct GameHealthComponent    GameHealthComponent;
typedef struct GameMovementComponent  GameMovementComponent;
typedef struct GameCollisionComponent GameCollisionComponent;
typedef struct GameCameraComponent    GameCameraComponent;

static constexpr char NIH_PLAYER_CLASS_NAME[] = "qm1_player";

typedef enum NihPlayerAudioChannel
{
	NIH_PLAYER_AUDIO_CHANNEL_VOICE,
	NIH_PLAYER_AUDIO_CHANNEL_FOOTSTEP,

	NIH_PLAYER_MAX_AUDIO_CHANNELS
} NihPlayerAudioChannel;

typedef struct NihPlayerEntity
{
	GamePlayer *player;

	GameHealthComponent    *healthComponent;
	GameMovementComponent  *movementComponent;
	GameCollisionComponent *collisionComponent;
	GameCameraComponent    *cameraComponent;

	ApeModelNode *model;

	ApeAudioSource *audioSources[ NIH_PLAYER_MAX_AUDIO_CHANNELS ];
} NihPlayerEntity;

#define NIH_PLAYER_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), NIH_PLAYER_CLASS_NAME, NihPlayerEntity )

void nih_entity_player_set_camera_state( const ApeEntity *self, GameCameraState state );
void nih_entity_player_toggle_camera_state( const ApeEntity *self );
