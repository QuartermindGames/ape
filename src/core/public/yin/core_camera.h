// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Camera APIs

#pragma once

typedef enum ApeCameraMode {
	APE_CAMERA_MODE_PERSPECTIVE,
	APE_CAMERA_MODE_TOP,
	APE_CAMERA_MODE_LEFT,
	APE_CAMERA_MODE_FRONT,

	APE_CAMERA_MAX_MODES
} ApeCameraMode;

typedef enum ApeCameraDrawMode {
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

ApeCamera *arl_camera_create( const char *tag, const PLVector3 *position, const PLVector3 *angles );
void arl_camera_destroy( ApeCamera *camera );
void arl_camera_set_position( ApeCamera *camera, const PLVector3 *position );
void arl_camera_set_angles( ApeCamera *camera, const PLVector3 *angles );

PLVector3 arl_camera_get_position( const ApeCamera *camera );
PLVector3 arl_camera_get_angles( const ApeCamera *camera );
PLVector3 arl_camera_get_forward( const ApeCamera *camera );

ApeCamera *arl_camera_get_active( void );
void arl_camera_make_active( ApeCamera *camera );

PL_EXTERN_C_END
