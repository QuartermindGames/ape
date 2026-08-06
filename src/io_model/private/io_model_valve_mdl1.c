// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Valve MDL loader (GoldSrc)
// Author:  Mark E. Sowden

#include "qmmath/public/qm_math_vector.h"

#include "io_model/public/io_model.h"

static constexpr unsigned int VMDL1_MAX_BONE_CONTROLLERS = 6;

typedef struct VMdl1Header
{
	uint32_t magic;
	uint32_t version;

	char     name[ 64 ];
	uint32_t length;

	QmMathVector3f eyePosition;
	QmMathVector3f min;
	QmMathVector3f max;

	QmMathVector3f bbMin;
	QmMathVector3f bbMax;

	int32_t flags;

	uint32_t numBones;
	uint32_t boneOffset;

	uint32_t numBoneControllers;
	uint32_t boneControllerOffset;

	uint32_t numHitBoxes;
	uint32_t hitBoxOffset;

	uint32_t numSequences;
	uint32_t sequenceOffset;

	uint32_t numSequenceGroups;
	uint32_t sequenceGroupsOffset;

	uint32_t numTextures;
	uint32_t textureOffset;
	uint32_t textureDataOffset;

	uint32_t numSkinReferences;
	uint32_t numSkinFamilies;
	uint32_t skinOffset;

	uint32_t numBodyParts;
	uint32_t bodyPartsOffset;

	uint32_t numAttachments;
	uint32_t attachmentsOffset;

	uint32_t soundTable;
	uint32_t soundIndex;
	uint32_t soundGroups;
	uint32_t soundGroupsOffset;

	uint32_t numTransitions;
	uint32_t transitionsOffset;
} VMdl1Header;

typedef struct VMdl1SequenceHeader
{
	uint32_t magic;
	uint32_t version;

	char     name[ 64 ];
	uint32_t length;
} VMdl1SequenceHeader;

typedef struct VMdl1Bone
{
	char    name[ 32 ];
	int32_t parent;
	int32_t flags;
	int32_t boneControllers[ VMDL1_MAX_BONE_CONTROLLERS ];
	float   value[ VMDL1_MAX_BONE_CONTROLLERS ];
	float   scale[ VMDL1_MAX_BONE_CONTROLLERS ];
} VMdl1Bone;

typedef struct VMdl1BoneController
{
	int32_t  bone;
	uint32_t type;
	float    start;
	float    end;
	int32_t  rest;
	uint32_t index;
} VMdl1BoneController;
