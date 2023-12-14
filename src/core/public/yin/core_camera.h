// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Camera APIs

#pragma once

typedef enum SSArlCameraMode
{
	SS_ARL_CAMERA_MODE_INVALID = -1,

	// basic view modes
	SS_ARL_CAMERA_MODE_PERSPECTIVE,
	SS_ARL_CAMERA_MODE_ISOMETRIC,

	// editor modes
	SS_ARL_CAMERA_MODE_TOP,
	SS_ARL_CAMERA_MODE_LEFT,
	SS_ARL_CAMERA_MODE_FRONT,

	SS_ARL_CAMERA_MAX_MODES
} SSArlCameraMode;

typedef enum ApeCameraDrawMode
{
	// "basic" draw modes
	SS_ARL_CAMERA_DRAW_MODE_WIREFRAME,
	SS_ARL_CAMERA_DRAW_MODE_SOLID,
	SS_ARL_CAMERA_DRAW_MODE_TEXTURED,
	// and "complete" - uses material system
	SS_ARL_CAMERA_DRAW_MODE_SHADED,

	SS_ARL_CAMERA_MAX_DRAW_MODES
} ApeCameraDrawMode;


PL_EXTERN_C

typedef struct ApeWorld ApeWorld;
typedef struct SSAclWorldRoom SSAclWorldRoom;
typedef struct SSArlCamera SSArlCamera;

SSArlCamera *ss_arl_camera_create( const char *tag, const PLVector3 *position, const PLVector3 *angles, SSArlCameraMode cameraMode );
void ss_arl_camera_destroy( SSArlCamera *camera );
void ss_arl_camera_set_position( SSArlCamera *camera, const PLVector3 *position );
void ss_arl_camera_set_angles( SSArlCamera *camera, const PLVector3 *angles );

PLVector3 ss_arl_camera_get_position( const SSArlCamera *camera );
PLVector3 ss_arl_camera_get_angles( const SSArlCamera *camera );
PLVector3 ss_arl_camera_get_forward( const SSArlCamera *camera );

void ss_arl_camera_make_active( SSArlCamera *camera );

void ss_arl_camera_assign_world( SSArlCamera *camera, ApeWorld *world );
ApeWorld *ss_arl_camera_get_world( SSArlCamera *camera );

void ss_arl_camera_set_draw_mode( SSArlCamera *camera, ApeCameraDrawMode drawMode );
void ss_arl_camera_set_view_mode( SSArlCamera *camera, SSArlCameraMode viewMode );

PLGCamera *ss_arl_camera_get_internal( SSArlCamera *camera );

SSArlLight **ss_acl_camera_get_visible_lights_( SSArlCamera *camera, unsigned int *num );
SSAclWorldRoom **ss_acl_camera_get_visible_rooms_( SSArlCamera *camera, unsigned int *num );

/////////////////////////////////////////////////////////////////////////////////////

void ss_arl_build_camera_visibility_lists_( void );
void ss_arl_clear_camera_visibility_lists_( void );

PL_EXTERN_C_END
