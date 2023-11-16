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

typedef struct SS_Arl_Camera SS_Arl_Camera;

PL_EXTERN_C

SS_Arl_Camera *ss_arl_camera_create( const char *tag, const PLVector3 *position, const PLVector3 *angles );
void ss_arl_camera_destroy( SS_Arl_Camera *camera );
void ss_arl_camera_set_position( SS_Arl_Camera *camera, const PLVector3 *position );
void ss_arl_camera_set_angles( SS_Arl_Camera *camera, const PLVector3 *angles );

PLVector3 ss_arl_camera_get_position( const SS_Arl_Camera *camera );
PLVector3 ss_arl_camera_get_angles( const SS_Arl_Camera *camera );
PLVector3 ss_arl_camera_get_forward( const SS_Arl_Camera *camera );

SS_Arl_Camera *ss_arl_camera_get_active( void );
void ss_arl_camera_make_active( SS_Arl_Camera *camera );

PL_EXTERN_C_END
