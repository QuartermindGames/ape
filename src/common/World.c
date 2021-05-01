/* Copyright (C) Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_filesystem.h>

#include "common/World.h"

WLDHandle *WLD_ReadFile( PLFile *file ) {
	return NULL;
}

WLDHandle *WLD_LoadFile( const char *path ) {
	PLFile *file = PlOpenFile( path, false );
	WLDHandle *handle = WLD_ReadFile( file );
	PlCloseFile( file );
	return handle;
}

void WLD_DestroyHandle( WLDHandle *handle ) {

}
