// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Camera APIs

#pragma once

typedef enum OgeCameraMode
{
	OGE_CAMERA_MODE_PERSPECTIVE,
	OGE_CAMERA_MODE_TOP,
	OGE_CAMERA_MODE_LEFT,
	OGE_CAMERA_MODE_FRONT,

	OGE_CAMERA_MAX_MODES
} ApeCameraMode;

typedef enum OgeCameraDrawMode
{
	// "basic" draw modes
	OGE_CAMERA_DRAW_MODE_WIREFRAME,
	OGE_CAMERA_DRAW_MODE_SOLID,
	OGE_CAMERA_DRAW_MODE_TEXTURED,
	// and "complete" - uses material system
	OGE_CAMERA_DRAW_MODE_SHADED,

	OGE_CAMERA_MAX_DRAW_MODES
} ApeCameraDrawMode;

typedef struct ApeCamera ApeCamera;

PL_EXTERN_C

ApeCamera *ogeCreateCamera( const char *tag, const PLVector3 *position, const PLVector3 *angles );
void ogeDestroyCamera( ApeCamera *camera );
void ogeSetCameraPosition( ApeCamera *camera, const PLVector3 *position );
void ogeSetCameraAngles( ApeCamera *camera, const PLVector3 *angles );

PLVector3 ogeGetCameraPosition( ApeCamera *camera );
PLVector3 ogeGetCameraAngles( ApeCamera *camera );
PLVector3 ogeGetCameraForward( ApeCamera *camera );

ApeCamera *ogeGetActiveCamera( void );
void ogeMakeCameraActive( ApeCamera *camera );

PL_EXTERN_C_END
