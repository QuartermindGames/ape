/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plmodel/plm.h>

#include <yin/node.h>

/* todo: add verbose mode */
#define Print( ... ) printf( __VA_ARGS__ )
#define Error( ... )           \
	{                          \
		printf( __VA_ARGS__ ); \
		exit( EXIT_FAILURE );  \
	}

typedef struct PLImage PLImage;

typedef enum PKGFileType {
	PKG_FILETYPE_GENERIC,
	PKG_FILETYPE_TEXTURE,
	PKG_FILETYPE_MODEL,
} PKGFileType;

/* pack_model.c */
NdBranch *MDL_ConvertPlatformModelToNodeModel( const PLMModel *model );
PLMModel *MDL_SMD_LoadFile( const char *path );

/* pack_image.c */
void PackImage_Write( const char *path, const PLImage *image, uint8_t destFormat );
