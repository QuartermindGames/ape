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
} OgeCameraMode;

typedef enum OgeCameraDrawMode
{
	// "basic" draw modes
	OGE_CAMERA_DRAW_MODE_WIREFRAME,
	OGE_CAMERA_DRAW_MODE_SOLID,
	OGE_CAMERA_DRAW_MODE_TEXTURED,
	// and "complete" - uses material system
	OGE_CAMERA_DRAW_MODE_SHADED,

	OGE_CAMERA_MAX_DRAW_MODES
} OgeCameraDrawMode;

typedef struct OgeCamera OgeCamera;

PL_EXTERN_C

OgeCamera *ogeCamera_Create( const char *tag, const PLVector3 *position, const PLVector3 *angles );
void ogeCamera_Destroy( OgeCamera *camera );
void ogeCamera_SetPosition( OgeCamera *camera, const PLVector3 *position );
void ogeCamera_SetAngles( OgeCamera *camera, const PLVector3 *angles );

OgeCamera *ogeGetActiveCamera( void );
void ogeMakeCameraActive( OgeCamera *camera );

PL_EXTERN_C_END
