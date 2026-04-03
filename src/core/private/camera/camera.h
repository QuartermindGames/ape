// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_public_camera.h"

typedef struct PLGCamera PLGCamera;

static constexpr unsigned int APE_CAMERA_MAX_PORTAL_DEPTH = 8;// maximum depth into a portal

static constexpr unsigned int APE_CAMERA_MAX_ROOMS        = 8;   // maximum rooms visible at a time
static constexpr unsigned int APE_CAMERA_MAX_ROOM_PORTALS = 16;  // maximum number of portals visible at a time, per room
static constexpr unsigned int APE_CAMERA_MAX_ROOM_LIGHTS  = 512; // maximum lights visible at a time, per room
static constexpr unsigned int APE_CAMERA_MAX_ROOM_NODES   = 8192;// maximum nodes visible at a time, per room

typedef struct ApeCameraVisibleSet    ApeCameraVisibleSet;
typedef struct ApeCameraVisiblePortal ApeCameraVisiblePortal;
typedef struct ApeCameraVisibleRoom   ApeCameraVisibleRoom;

typedef struct ApeCameraVisiblePortal
{
	ApeBrushFace *portalFace;

	ApeCameraVisibleRoom *nextRoom;

	QmMathVector3f origin;
	QmMathVector3f normal;

	QmMathVector4f screenRect;// area of the screen the portal occupies
} ApeCameraVisiblePortal;

typedef struct ApeCameraVisibleRoom
{
	ApeCameraVisiblePortal *entrance;

	ApeRoom *room;

	ApeLight    *lights[ APE_CAMERA_MAX_ROOM_LIGHTS ];
	unsigned int numLights;

	ApeWorldNode *nodes[ APE_CAMERA_MAX_ROOM_NODES ];
	unsigned int  numNodes;

	ApeCameraVisiblePortal portals[ APE_CAMERA_MAX_ROOM_PORTALS ];// array of visible portals
	unsigned int           numPortals;                            // number of visible portals

	PLMatrix4 viewMatrix;
} ApeCameraVisibleRoom;

typedef struct ApeCameraVisibleSet
{
	ApeCameraVisibleRoom rooms[ APE_CAMERA_MAX_ROOMS ];
	unsigned int         numRooms;
} ApeCameraVisibleSet;

enum
{
	APE_CAMERA_FRUSTUM_PLANE_RIGHT,
	APE_CAMERA_FRUSTUM_PLANE_LEFT,
	APE_CAMERA_FRUSTUM_PLANE_BOTTOM,
	APE_CAMERA_FRUSTUM_PLANE_TOP,
	APE_CAMERA_FRUSTUM_PLANE_FAR,
	APE_CAMERA_FRUSTUM_PLANE_NEAR,

	APE_CAMERA_MAX_FRUSTUM_PLANES
};

typedef QmMathVector4f ApeCameraViewFrustum[ APE_CAMERA_MAX_FRUSTUM_PLANES ];

typedef struct ApeCamera
{
	// This should always come first!
	ApeWorldNode base;

	char tag[ 32 ];

	struct
	{
		float focusPoint;
		float focusScale;
		float aperture;
	} dof;

	bool active;

	float fov;
	float near, far;

	PLMatrix4 proj;
	PLMatrix4 view;

	ApeCameraViewFrustum frustum;

	ApeCameraViewMode mode;
	ApeCameraDrawMode drawMode;

	ApeCameraVisibleSet pvs;
} ApeCamera;

void ape_camera_build_pvs_( ApeCamera *self, const ApeViewport *viewport );
