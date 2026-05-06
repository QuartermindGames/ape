// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmmath/public/qm_math_vector.h"

typedef struct ApeCamera ApeCamera;

typedef enum GameCameraState
{
	GAME_CAMERA_STATE_FREE,        // free camera
	GAME_CAMERA_STATE_FIXED,       // game has control over it
	GAME_CAMERA_STATE_FIRST_PERSON,// first-person w/ input
	GAME_CAMERA_STATE_THIRD_PERSON,// third-person w/ input

	GAME_CAMERA_STATE_MAX,
} GameCameraState;

typedef struct GameCameraComponent
{
	GameCameraState state;
	GameCameraState oldState;

	float          distance;// distance from the entity and camera
	float          side;    // how far the camera should shift left or right
	float          height;  // height from origin of entity
	QmMathVector3f angles;  // orbital rotation around the entity
} GameCameraComponent;

static constexpr char GAME_CAMERA_COMPONENT_NAME[] = "camera";

void game_component_camera_handle_input_( GameCameraComponent *component, double delta );
void game_component_camera_tick_( const GameCameraComponent *component, ApeCamera *camera, double delta );
void game_component_camera_cycle_state_( GameCameraComponent *component );
