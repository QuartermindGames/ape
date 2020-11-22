/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <PL/platform_filesystem.h>

#include "core/wld.h"

WldHandle *WLD_ReadFile( PLFile *file ) {
	return NULL;
}

WldHandle *WLD_LoadFile( const char *path ) {
	PLFile *file = plOpenFile( path, false );
	WldHandle *handle = WLD_ReadFile( file );
	plCloseFile( file );
	return handle;
}

void WLD_DestroyHandle( WldHandle *handle ) {

}
