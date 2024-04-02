// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../shared/game_private.h"

typedef enum SS3CameraMode
{
	SS3_CAMERA_MODE_FREE,
	SS3_CAMERA_MODE_SPECTATE,
	SS3_CAMERA_MODE_FOLLOW,
	SS3_CAMERA_MODE_ATTACHED,

	SS3_MAX_CAMERA_MODES
} SS3CameraMode;
