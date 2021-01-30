/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/platform_model.h>

#include "yin.h"

typedef struct GSMDLHeader {
	int32_t magic;
	int32_t version;

	char name[ 64 ];
	int32_t length;
	PLVector3 eyePosition;
	PLVector3 min;
	PLVector3 max;
	PLVector3 bbMin;
	PLVector3 bbMax;
	int32_t flags;
	int32_t numBones;
	int32_t boneIndex;
	int32_t numBoneControllers;
	int32_t boneControllerIndex;
	int32_t numHitBoxes;
	int32_t hitBoxIndex;
	int32_t numAnimations;
	int32_t animationIndex;
	int32_t numAnimationGroups;
	int32_t animationGroupIndex;
	int32_t numTextures;
	int32_t textureIndex;
	int32_t textureDataIndex;
	int32_t numSkinReferences;
	int32_t numSkinFamilies;
	int32_t skinIndex;
	int32_t numBodyParts;
	int32_t bodyPartIndex;
	int32_t numAttachments;
	int32_t attachmentIndex;
	int32_t soundTable;
	int32_t soundIndex;
	int32_t soundGroups;
	int32_t soundGroupIndex;
	int32_t numTransitions;
	int32_t transitionIndex;
} GSMDLHeader;

PLModel *GSMDL_LoadFile( const char *path ) {
	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}

    GSMDLHeader header;
	plReadFile( file, &header, sizeof( GSMDLHeader ), 1 );

	return NULL;
}
