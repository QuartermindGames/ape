/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plmodel/plm.h>

#include "yin.h"

#define MAX_NAME             64
#define MAX_LABEL            32
#define MAX_BONE_CONTROLLERS 6

#define MDL_MAGIC     MAGIC_TO_NUM( 'I', 'D', 'S', 'T' )
#define MDL_SEQ_MAGIC MAGIC_TO_NUM( 'I', 'D', 'S', 'Q' )

#define MDL_VERSION 10

typedef struct VMDLHeader
{
	int32_t   magic;// IDST / IDSQ
	int32_t   version;
	char      name[ MAX_NAME ];
	int32_t   length;
	PLVector3 eyePos;
	PLVector3 min;
	PLVector3 max;
	PLVector3 bbMin;
	PLVector3 bbMax;
	int32_t   flags;
	int32_t   numBones;
	int32_t   boneIndex;
	int32_t   numBoneControllers;
	int32_t   boneControllerIndex;
	int32_t   numHitBoxes;
	int32_t   hitBoxIndex;
	int32_t   numAnims;
	int32_t   animIndex;
	int32_t   numAnimGroups;
	int32_t   animGroupIndex;
	int32_t   numTextures;
	int32_t   textureIndex;
	int32_t   textureDataIndex;
	int32_t   numSkinRefs;
	int32_t   numSkinFamilies;
	int32_t   skinIndex;
	int32_t   numBodyParts;
	int32_t   bodyPartIndex;
	int32_t   numAttachments;
	int32_t   attachmentIndex;
	int32_t   soundTable;
	int32_t   soundIndex;
	int32_t   numSoundGroups;
	int32_t   soundGroupIndex;
	int32_t   numTransitions;
	int32_t   transitionIndex;
} VMDLHeader;

typedef struct VMDLBoundingBox
{
	int32_t   bone;
	int32_t   group;
	PLVector3 bbMin;
	PLVector3 bbMax;
} VMDLBoundingBox;

typedef struct VMDLAnimationHeader
{
	int32_t id;
	int32_t version;
	char    name[ MAX_NAME ];
	int32_t length;
} VMDLAnimationHeader;

typedef struct VMDLAnimationGroup
{
	char    label[ MAX_LABEL ];
	char    name[ MAX_NAME ];
	int32_t unused[ 2 ];
} VMDLAnimationGroup;

typedef struct VMDLBone
{
	char    name[ MAX_LABEL ];
	int32_t parent;
	int32_t flags;
	int32_t boneControllers[ MAX_BONE_CONTROLLERS ];
	float   values[ MAX_BONE_CONTROLLERS ];
	float   scale[ MAX_BONE_CONTROLLERS ];
} VMDLBone;

typedef struct VMDLBoneController
{
	int32_t bone;
	int32_t type;
	float   range[ 2 ];
	int32_t rest;
	int32_t index;
} VMDLBoneController;

PLMModel *GSMDL_LoadFile( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		return NULL;
	}

	VMDLHeader header;
	PlReadFile( file, &header, sizeof( VMDLHeader ), 1 );
	/* now carry out some basic validation */

	if ( header.magic != MDL_MAGIC && header.magic != MDL_SEQ_MAGIC )
	{
		PrintWarn( "Invalid identifier for MDL: %d vs %d!\n", header.magic, MDL_MAGIC );
		return NULL;
	}
}
