// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmos/public/qm_os.h"

#include "qmmath/public/qm_math.h"
#include "qmmath/public/qm_math_vector.h"

typedef enum IOModelResultCode : int8_t
{
	IO_MODEL_RESULT_CODE_HEAD = INT8_MIN,
	IO_MODEL_RESULT_CODE_IO_ERROR,
	IO_MODEL_RESULT_CODE_VERSION_ERROR,
	IO_MODEL_RESULT_CODE_UNSUPPORTED_ERROR,
	IO_MODEL_RESULT_CODE_SUCCESS = 0,
} IOModelResultCode;

typedef struct IOModelResult
{
	const char *str;
	int8_t      code;
} IOModelResult;

#define IO_MODEL_RESULT( R, STR, CODE ) \
	{                                   \
		if ( ( R ) != nullptr )         \
		{                               \
			( R )->str  = ( STR );      \
			( R )->code = ( CODE );     \
		}                               \
	}
#define IO_MODEL_RESULT_SUCCESS( R ) IO_MODEL_RESULT( R, "success", IO_MODEL_RESULT_CODE_SUCCESS )

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

typedef enum IOModelAnimationFlag : uint8_t
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

/**
 * Used for skeletal animation types, of course.
 */
typedef struct IOModelSkeletal
{
} IOModelSkeletal;

/**
 * This is basically for per-vertex animation.
 * I've called it morph just to avoid getting muddled.
 */
typedef struct IOModelMorph
{
	unsigned int numFrames;
} IOModelMorph;

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

typedef struct IOModelTriangle
{
	//TODO: change to uint16
	uint32_t indices[ 3 ];
} IOModelTriangle;

typedef enum IOModelFlag : uint8_t
{
	QM_OS_BIT_FLAG( IO_MODEL_FLAG_ANIMATED, 0U ),// if the model doesn't have this, it's assumed static
} IOModelFlag;

typedef struct IOModelMesh
{
	uint8_t  materialIndex;
	uint16_t startTri;
	uint16_t endTri;
} IOModelMesh;

typedef struct IOModel
{
	char name[ IO_MODEL_MAX_NAME ];

	char   *materialPaths[ IO_MODEL_MAX_MATERIALS ];
	uint8_t numMaterials;

	IOModelVertex *vertices;
	unsigned int   numVertices;

	IOModelTriangle *triangles;
	unsigned int     numTriangles;

	IOModelMesh *meshes;
	unsigned int numMeshes;

	IOModelType type;
	union
	{
		IOModelSkeletal skeletal;

		struct
		{
			unsigned int  numMorphs;
			IOModelMorph *morphs;
		} morph;
	} typeData;
} IOModel;

typedef enum IOModelFileFormat : uint8_t
{
	IO_MODEL_FILE_FORMAT_ANY = ( uint8_t ) -1,

	//IO_MODEL_FILE_FORMAT_CYCLONE = 0,
	//IO_MODEL_FILE_FORMAT_HDV,// into the shadows
	//IO_MODEL_FILE_FORMAT_U3D,// outcast
	//IO_MODEL_FILE_FORMAT_OBJ,// autodesk obj
	//IO_MODEL_FILE_FORMAT_CPJ,
	//IO_MODEL_FILE_FORMAT_PLY,
	IO_MODEL_FILE_FORMAT_SMD = 0,// valve smd

	IO_MODEL_FILE_FORMAT_MAX,
} IOModelFileFormat;

[[nodiscard]] IOModel *io_model_load( const char *path, IOModelFileFormat format, IOModelResult *result );
