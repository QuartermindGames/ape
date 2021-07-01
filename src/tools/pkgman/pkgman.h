/* ======================================================================
 * PkgMan, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plmodel/plm.h>

#include <common/node.h>

#define Print( ... ) printf( __VA_ARGS__ )
#define Error( ... )           \
	{                          \
		printf( __VA_ARGS__ ); \
		exit( EXIT_FAILURE );  \
	}

typedef struct PLImage PLImage;

/* pack_model.c */
NLNode *  MDL_ConvertPlatformModelToNodeModel( const PLMModel *model );
PLMModel *MDL_STL_LoadFile( const char *path );
PLMModel *MDL_MD2_LoadFile( const char *path );
PLMModel *MDL_MDL_LoadFile( const char *path );
PLMModel *MDL_PLY_LoadFile( const char *path );

/* pack_image.c */
void PackImage_Write( const char *path, const PLImage *image, uint8_t destFormat );
