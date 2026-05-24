// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

static constexpr char NIH_PLAYER_CLASS_NAME[] = "qm1_player";

typedef enum SS1PlayerAudioChannel
{
	SS1_PLAYER_AUDIO_CHANNEL_VOICE,
	SS1_PLAYER_AUDIO_CHANNEL_FOOTSTEP,

	SS1_PLAYER_MAX_AUDIO_CHANNELS
} SS1PlayerAudioChannel;

typedef struct Qm1PlayerEntity
{
	GamePlayer *player;

	struct GameHealthComponent    *healthComponent;
	struct GameMovementComponent  *movementComponent;
	struct GameCollisionComponent *collisionComponent;
	struct GameCameraComponent    *cameraComponent;

	struct ApeAudioSource *audioSources[ SS1_PLAYER_MAX_AUDIO_CHANNELS ];
} Qm1PlayerEntity;

#define QM1_PLAYER_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), NIH_PLAYER_CLASS_NAME, Qm1PlayerEntity )
