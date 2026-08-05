// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmos/public/qm_os.h"

#include "qmmath/public/qm_math.h"
#include "qmmath/public/qm_math_vector.h"

static constexpr char         IO_MODEL_EXTENSION[] = "mdl";
static constexpr unsigned int IO_MODEL_VERSION     = 4;

// much of this is just here for sanity checking -
// in the long-term, we should really look at making
// these dynamically allocated instead...

static constexpr unsigned int IO_MODEL_MAX_NAME      = 64;
static constexpr unsigned int IO_MODEL_MAX_MATERIALS = 16;

static constexpr unsigned int IO_MODEL_MAX_BONE_NAME = 64;
static constexpr unsigned int IO_MODEL_MAX_BONES     = 256;
static constexpr unsigned int IO_MODEL_MAX_WEIGHTS   = 4;

static constexpr unsigned int IO_MODEL_MAX_TRIANGLES = 8192;
static constexpr unsigned int IO_MODEL_MAX_VERTICES  = IO_MODEL_MAX_TRIANGLES * 2;

typedef enum IOModelType
{
	IO_MODEL_TYPE_STATIC,
	IO_MODEL_TYPE_VERTEX,
	IO_MODEL_TYPE_SKELETAL,

	IO_MODEL_NUM_TYPES
} IOModelType;

/////////////////////////////////////////////////////////////////////////////////////
// Animation

typedef enum IOModelAnimationFlag
{
	QM_OS_BIT_FLAG( IO_MODEL_ANIMATION_FLAG_LOOPING, 0U ),
} IOModelAnimationFlag;

typedef struct IOModelAnimationFrame
{
	QmMathVector3f mins;
	QmMathVector3f maxs;
} IOModelAnimationFrame;

typedef struct IOModelAnimation
{
	char                 name[ 64 ];
	IOModelAnimationFlag flags;

	unsigned int numFrames;

	float speed;

	unsigned int numBones;
} IOModelAnimation;

/////////////////////////////////////////////////////////////////////////////////////

typedef struct IOModelSkeletal
{
} IOModelSkeletal;

typedef struct IOModelBone
{
	char           name[ IO_MODEL_MAX_BONE_NAME ];
	int            parent;
	QmMathVector3f rotation;
	QmMathVector3f position;
} IOModelBone;

typedef struct IOModelVertexWeight
{
	float        weight;
	unsigned int bone;
} IOModelVertexWeight;

typedef struct IOModelVertex
{
	QmMathVector3f position;
	QmMathVector3f normal;
	QmMathVector2f uv;

	IOModelVertexWeight weights[ IO_MODEL_MAX_WEIGHTS ];
	unsigned int        numWeights;
} IOModelVertex;

typedef struct IOModelMesh
{
	uint8_t  materialIndex;
	uint16_t startTri;
	uint16_t endTri;
} IOModelMesh;

typedef enum IOModelFlag
{
	QM_OS_BIT_FLAG( IO_MODEL_FLAG_ANIMATED, 0 ),// if the model doesn't have this, it's assumed static
} IOModelFlag;

typedef struct IOModel
{
	char  name[ IO_MODEL_MAX_NAME ];
	char *materialPath;

	IOModelType type;

	union
	{
		IOModelSkeletal skeletal;
	} typeData;
} IOModel;

typedef enum IOModelFileFormat
{
	QM_OS_BIT_FLAG( IO_MODEL_FILE_FORMAT_CYCLONE, 0 ),
	QM_OS_BIT_FLAG( IO_MODEL_FILE_FORMAT_HDV, 1 ),// into the shadows
	QM_OS_BIT_FLAG( IO_MODEL_FILE_FORMAT_U3D, 2 ),// outcast
	QM_OS_BIT_FLAG( IO_MODEL_FILE_FORMAT_OBJ, 3 ),// autodesk obj
	QM_OS_BIT_FLAG( IO_MODEL_FILE_FORMAT_CPJ, 4 ),
	QM_OS_BIT_FLAG( IO_MODEL_FILE_FORMAT_PLY, 5 ),
	QM_OS_BIT_FLAG( IO_MODEL_FILE_FORMAT_SMD, 6 ),// valve smd
} IOModelFileFormat;
