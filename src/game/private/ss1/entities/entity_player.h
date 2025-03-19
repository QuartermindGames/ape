// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef enum SS1PlayerAudioChannel
{
	SS1_PLAYER_AUDIO_CHANNEL_VOICE,
	SS1_PLAYER_AUDIO_CHANNEL_FOOTSTEP,

	SS1_PLAYER_MAX_AUDIO_CHANNELS
} SS1PlayerAudioChannel;

typedef struct SS1PlayerEntity
{
	GamePlayer       *player;
	SS1ProfessionType profession;

	struct GameHealthComponent    *healthComponent;
	struct GameMovementComponent  *movementComponent;
	struct GameCollisionComponent *collisionComponent;

	struct ApeAudioSource *audioSources[ SS1_PLAYER_MAX_AUDIO_CHANNELS ];
	struct ApeModelNode   *model;

	float     cameraDistance;// distance from the entity and camera
	float     cameraSide;    // how far the camera should shift left or right
	float     cameraHeight;  // height from origin of entity
	PLVector3 cameraAngles;  // orbital rotation around the entity
} SS1PlayerEntity;

#define SS1_PLAYER_ENTITY( SELF ) APE_ENT_CLASS( ( SELF ), SS1PlayerEntity )
