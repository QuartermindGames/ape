// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Camera APIs

#pragma once

#include "core_world.h"
typedef enum ApeCameraViewMode
{
	APE_CAMERA_MODE_INVALID = -1,
	APE_CAMERA_MODE_PERSPECTIVE,
	APE_CAMERA_MODE_TOP,
	APE_CAMERA_MODE_LEFT,
	APE_CAMERA_MODE_FRONT,

	APE_CAMERA_MODE_ISOMETRIC,

	APE_CAMERA_MAX_MODES
} ApeCameraViewMode;

typedef enum ApeCameraDrawMode
{
	APE_CAMERA_DRAW_MODE_INVALID = -1,

	// "basic" draw modes
	APE_CAMERA_DRAW_MODE_WIREFRAME,
	APE_CAMERA_DRAW_MODE_SOLID,
	APE_CAMERA_DRAW_MODE_TEXTURED,
	// and "complete" - uses material system
	APE_CAMERA_DRAW_MODE_SHADED,

	APE_CAMERA_MAX_DRAW_MODES
} ApeCameraDrawMode;

PL_EXTERN_C

typedef struct ApeWorld  ApeWorld;
typedef struct ApeRoom   ApeRoom;
typedef struct ApeCamera ApeCamera;

ApeCamera *ape_create_camera( ApeWorldNode *parent, const char *name, const PLVector3 *position, const PLVector3 *angles, ApeCameraViewMode cameraMode, ApeCameraDrawMode drawMode );
void       ape_camera_destroy( ApeCamera *camera );
void       ape_camera_set_position( ApeCamera *self, const PLVector3 *position );
void       ape_camera_set_angles( ApeCamera *camera, const PLVector3 *angles );

PLVector3 ape_camera_get_position( const ApeCamera *camera );
PLVector3 ape_camera_get_angles( const ApeCamera *camera );
PLVector3 ape_camera_get_forward( const ApeCamera *camera );

void ape_camera_make_active( ApeCamera *camera );

ApeWorld *ape_camera_get_world( ApeCamera *self );
ApeRoom  *ape_camera_get_room( ApeCamera *self );

void ape_camera_set_room( ApeCamera *self, ApeRoom *room );

void ape_camera_set_draw_mode( ApeCamera *camera, ApeCameraDrawMode drawMode );
void ape_camera_set_view_mode( ApeCamera *camera, ApeCameraViewMode viewMode );

const char *ape_get_camera_draw_mode_label( ApeCameraDrawMode drawMode );
const char *ape_get_camera_view_mode_label( ApeCameraViewMode viewMode );

struct PLGCamera *ape_camera_get_internal( ApeCamera *camera );

ApeLight     **ape_camera_get_visible_lights_( ApeCamera *camera, unsigned int *num );
ApeRoom      **ape_camera_get_visible_rooms_( ApeCamera *camera, unsigned int *num );
ApeWorldNode **ape_camera_get_visible_nodes_( ApeCamera *self, unsigned int *num );
ApeBrushFace **ape_camera_get_visible_portals_( ApeCamera *self, unsigned int *num );

void ape_camera_build_pvs_( ApeCamera *self );

/////////////////////////////////////////////////////////////////////////////////////

void ape_build_camera_visibility_lists_( void );
void ape_clear_camera_visibility_lists_( void );

PL_EXTERN_C_END
