// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Camera APIs

#pragma once

typedef enum ApeCameraMode
{
	APE_CAMERA_MODE_PERSPECTIVE,
	APE_CAMERA_MODE_TOP,
	APE_CAMERA_MODE_LEFT,
	APE_CAMERA_MODE_FRONT,

	APE_CAMERA_MAX_MODES
} ApeCameraMode;

typedef enum ApeCameraDrawMode
{
	// "basic" draw modes
	APE_CAMERA_DRAW_MODE_WIREFRAME,
	APE_CAMERA_DRAW_MODE_SOLID,
	APE_CAMERA_DRAW_MODE_TEXTURED,
	// and "complete" - uses material system
	APE_CAMERA_DRAW_MODE_SHADED,

	APE_CAMERA_MAX_DRAW_MODES
} ApeCameraDrawMode;

typedef struct ApeCamera ApeCamera;

PL_EXTERN_C

ApeCamera *apeCreateCamera( const char *tag, const PLVector3 *position, const PLVector3 *angles );
void apeDestroyCamera( ApeCamera *camera );
void apeSetCameraPosition( ApeCamera *camera, const PLVector3 *position );
void apeSetCameraAngles( ApeCamera *camera, const PLVector3 *angles );

PLVector3 apeGetCameraPosition( ApeCamera *camera );
PLVector3 apeGetCameraAngles( ApeCamera *camera );
PLVector3 apeGetCameraForward( ApeCamera *camera );

ApeCamera *apeGetActiveCamera( void );
void apeMakeCameraActive( ApeCamera *camera );

PL_EXTERN_C_END
