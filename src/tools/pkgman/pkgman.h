/* ======================================================================
 * PkgMan, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#define Print( ... )	printf( __VA_ARGS__ )
#define Error( ... )	printf( __VA_ARGS__ ); exit( EXIT_FAILURE )

typedef struct PLImage PLImage;

/* pack_image.c */
void PackImage_Write( const char *path, const PLImage *image, uint8_t destFormat );
