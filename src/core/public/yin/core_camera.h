// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Camera APIs

#pragma once

typedef enum SSArlCameraMode
{
	SS_ARL_CAMERA_MODE_INVALID = -1,
	SS_ARL_CAMERA_MODE_PERSPECTIVE,
	APE_CAMERA_MODE_TOP,
	APE_CAMERA_MODE_LEFT,
	SS_ARL_CAMERA_MODE_FRONT,

	APE_CAMERA_MAX_MODES
} SSArlCameraMode;

typedef enum ApeCameraDrawMode
{
	// "basic" draw modes
	SS_ARL_CAMERA_DRAW_MODE_WIREFRAME,
	APE_CAMERA_DRAW_MODE_SOLID,
	SS_ARL_CAMERA_DRAW_MODE_TEXTURED,
	// and "complete" - uses material system
	APE_CAMERA_DRAW_MODE_SHADED,

	APE_CAMERA_MAX_DRAW_MODES
} ApeCameraDrawMode;

typedef struct SSArlCamera SSArlCamera;

PL_EXTERN_C

SSArlCamera *ss_arl_camera_create( const char *tag, const PLVector3 *position, const PLVector3 *angles, SSArlCameraMode cameraMode );
void ss_arl_camera_destroy( SSArlCamera *camera );
void ss_arl_camera_set_position( SSArlCamera *camera, const PLVector3 *position );
void ss_arl_camera_set_angles( SSArlCamera *camera, const PLVector3 *angles );

PLVector3 ss_arl_camera_get_position( const SSArlCamera *camera );
PLVector3 ss_arl_camera_get_angles( const SSArlCamera *camera );
PLVector3 ss_arl_camera_get_forward( const SSArlCamera *camera );

SSArlCamera *ss_arl_camera_get_active( void );
void ss_arl_camera_make_active( SSArlCamera *camera );

void ss_arl_camera_set_draw_mode( SSArlCamera *camera, ApeCameraDrawMode drawMode );
void ss_arl_camera_set_view_mode( SSArlCamera *camera, SSArlCameraMode viewMode );

PL_EXTERN_C_END
