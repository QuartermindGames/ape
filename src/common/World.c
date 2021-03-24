/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <PL/platform_filesystem.h>

#include "common/World.h"

WLDHandle *WLD_ReadFile( PLFile *file ) {
	return NULL;
}

WLDHandle *WLD_LoadFile( const char *path ) {
	PLFile *file = plOpenFile( path, false );
	WLDHandle *handle = WLD_ReadFile( file );
	plCloseFile( file );
	return handle;
}

void WLD_DestroyHandle( WLDHandle *handle ) {

}
