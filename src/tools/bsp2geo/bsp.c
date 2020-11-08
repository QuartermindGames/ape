/* Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <PL/platform.h>
#include <PL/platform_filesystem.h>

#include "bsp.h"

static BSPHandle *BSP_ReadFile( PLFile *file ) {
	BSPHeader header;
	if ( plReadFile( file, &header, sizeof( BSPHeader ), 1 ) != 1 ) {
		printf( "Failed to read in header!\n" );
		return NULL;
	}


}

BSPHandle *BSP_LoadFile( const char *path ) {
	PLFile *file = plOpenFile( path, false );
	if ( file == NULL ) {
		printf( "Failed to open BSP file, \"%s\"!\nPL: %s\n", path, plGetError() );
		return NULL;
	}

	BSPHandle *handle = BSP_ReadFile( file );

	plCloseFile( file );

	return handle;
}
